#include <Windows.h>
#include <sddl.h>
#include <aclapi.h>
#include <comdef.h>  // for _com_error
#include <iostream>
#include <string>


static void WIN32_CHECK(BOOL res) {
    if (res)
        return;

    _com_error error(GetLastError());
#ifdef _UNICODE
    const wchar_t * w_msg = error.ErrorMessage();
#pragma warning(push)
#pragma warning(disable: 4996) // function or variable may be unsafe
    std::string msg(wcslen(w_msg), '\0');
    wcstombs(const_cast<char*>(msg.data()), w_msg, msg.size());
#pragma warning(pop)
#else
    const char * msg = error.ErrorMessage();
#endif
    throw std::runtime_error(msg);
}

/// sporadic error codes experienced with SetNamedSecurityInfo
const static DWORD SPORADIC_FAILURE_CODES[]{
    ERROR_INSUFFICIENT_BUFFER, ///< 122
    ERROR_INVALID_SID,         ///< 1337
    ERROR_BAD_INHERITANCE_ACL, ///< 1340
};


/** Tag a folder path as writable by low-integrity processes.
    By default, only %USER PROFILE%\AppData\LocalLow is writable.
    Based on "Designing Applications to Run at a Low Integrity Level" https://msdn.microsoft.com/en-us/library/bb625960.aspx */
static DWORD MakePathLowIntegrity(const WCHAR * path) {
    DWORD ret = 0;
    do {
        ACL * sacl = nullptr; // system access control list
        {
            // initialize "low integrity" System Access Control List (SACL)
            PSECURITY_DESCRIPTOR SD = nullptr;
            // Security Descriptor String interpretation: (based on sddl.h)
            // SACL:(ace_type=Integrity label; ace_flags=; rights=SDDL_NO_WRITE_UP; object_guid=; inherit_object_guid=; account_sid=Low mandatory level)
            WIN32_CHECK(ConvertStringSecurityDescriptorToSecurityDescriptorW(L"S:(ML;;NW;;;LW)", SDDL_REVISION_1, &SD, NULL));
            BOOL sacl_present = FALSE;
            BOOL sacl_defaulted = FALSE;
            WIN32_CHECK(GetSecurityDescriptorSacl(SD, &sacl_present, &sacl, &sacl_defaulted));
            LocalFree(SD);
        }

        // apply "low integrity" SACL
        ret = SetNamedSecurityInfoW(const_cast<WCHAR*>(path), SE_FILE_OBJECT, LABEL_SECURITY_INFORMATION, /*owner*/NULL, /*group*/NULL, /*Dacl*/NULL, sacl);
        if (ret == ERROR_SUCCESS)
            return ret;

        bool sporadic_failure = std::find(std::begin(SPORADIC_FAILURE_CODES), std::end(SPORADIC_FAILURE_CODES), ret) != std::end(SPORADIC_FAILURE_CODES);
        if (!sporadic_failure)
            return ret;
    } while (true);

    // ERROR_FILE_NOT_FOUND ///< 2
    // ERROR_ACCESS_DENIED  ///< 5
    return ret; // failure
}

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 2) {
        std::wcout << L"Utility to make filesystem paths writable from low-integrity processes.\n";
        std::wcout << L"Usage: MakeLowIntegrity <path>\n";
        return 1;
    }

    std::wstring path = argv[1];
    std::wcout << L"Making path low-integrity: " << path << std::endl;

    DWORD err = MakePathLowIntegrity(path.c_str());
    if (!err) {
        std::wcout << L"Success." << std::endl;
        return 0; // success
    }

    std::wcout << L"ERROR code: " << err << std::endl;
    return 2;
}
