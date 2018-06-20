#pragma once

namespace plog
{
    template <class Formatter>
    class EventLogAppender : public IAppender
    {
    public:
        EventLogAppender(const wchar_t* sourceName) : 
            m_eventSource(RegisterEventSourceW(NULL, sourceName)),
            m_eventCategory(0),
            m_eventMessageId(0)
        {
        }

        EventLogAppender(const wchar_t* sourceName, const WORD eventCategory, const DWORD eventMessageId) :
            m_eventSource(RegisterEventSourceW(NULL, sourceName)),
            m_eventCategory(eventCategory),
            m_eventMessageId(eventMessageId)
            
        {
        }

        ~EventLogAppender()
        {
            DeregisterEventSource(m_eventSource);
        }

        virtual void write(const Record& record)
        {
            std::wstring str = Formatter::format(record);
            const wchar_t* logMessagePtr[] = { str.c_str() };

            ReportEventW(m_eventSource, 
                logSeverityToType(record.getSeverity()), 
                m_eventCategory ? m_eventCategory : static_cast<WORD>(record.getSeverity()),
                m_eventMessageId, 
                NULL, 
                1,
                0, 
                logMessagePtr, 
                NULL);
        }

    private:
        static WORD logSeverityToType(plog::Severity severity)
        {
            switch (severity)
            {
            case plog::fatal:
            case plog::error:
                return eventLog::kErrorType;

            case plog::warning:
                return eventLog::kWarningType;

            case plog::info:
            case plog::debug:
            case plog::verbose:
            default:
                return eventLog::kInformationType;
            }
        }

    private:
        HANDLE m_eventSource;
        WORD m_eventCategory;
        DWORD m_eventMessageId;
    };

    class EventLogAppenderRegistry
    {
    public:
        static bool add(
            const wchar_t* sourceName,
            const wchar_t* logName = L"Application",
            const wchar_t* eventMessageFile = L"%windir%\\Microsoft.NET\\Framework\\v4.0.30319\\EventLogMessages.dll;%windir%\\Microsoft.NET\\Framework\\v2.0.50727\\EventLogMessages.dll",
            const WORD categoryCount = 0)
        {
            std::wstring logKeyName;
            std::wstring sourceKeyName;
            getKeyNames(sourceName, logName, sourceKeyName, logKeyName);

            HKEY sourceKey;
            if (0 != RegCreateKeyExW(hkey::kLocalMachine, sourceKeyName.c_str(), 0, NULL, 0, regSam::kSetValue, NULL, &sourceKey, NULL))
            {
                return false;
            }

            const DWORD kTypesSupported = eventLog::kErrorType | eventLog::kWarningType | eventLog::kInformationType;
            RegSetValueExW(sourceKey, L"TypesSupported", 0, regType::kDword, &kTypesSupported, sizeof(kTypesSupported));
            
            if (categoryCount > 0)
            {
                RegSetValueExW(sourceKey, L"CategoryCount", 0, regType::kDword, reinterpret_cast<const BYTE*>(&categoryCount), sizeof(DWORD));
                RegSetValueExW(sourceKey, L"CategoryMessageFile", 0, regType::kExpandSz, reinterpret_cast<const BYTE*>(eventMessageFile), static_cast<DWORD>(::wcslen(eventMessageFile) * sizeof(wchar_t)));
            }

            RegSetValueExW(sourceKey, L"EventMessageFile", 0, regType::kExpandSz, reinterpret_cast<const BYTE*>(eventMessageFile), static_cast<DWORD>(::wcslen(eventMessageFile) * sizeof(wchar_t)));

            RegCloseKey(sourceKey);
            return true;
        }

        static bool exists(const wchar_t* sourceName, const wchar_t* logName = L"Application")
        {
            std::wstring logKeyName;
            std::wstring sourceKeyName;
            getKeyNames(sourceName, logName, sourceKeyName, logKeyName);

            HKEY sourceKey;
            if (0 != RegOpenKeyExW(hkey::kLocalMachine, sourceKeyName.c_str(), 0, regSam::kQueryValue, &sourceKey))
            {
                return false;
            }

            RegCloseKey(sourceKey);
            return true;
        }

        static void remove(const wchar_t* sourceName, const wchar_t* logName = L"Application")
        {
            std::wstring logKeyName;
            std::wstring sourceKeyName;
            getKeyNames(sourceName, logName, sourceKeyName, logKeyName);

            RegDeleteKeyW(hkey::kLocalMachine, sourceKeyName.c_str());
            RegDeleteKeyW(hkey::kLocalMachine, logKeyName.c_str());
        }

    private:
        static void getKeyNames(const wchar_t* sourceName, const wchar_t* logName, std::wstring& sourceKeyName, std::wstring& logKeyName)
        {
            const std::wstring kPrefix = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\";
            logKeyName = kPrefix + logName;
            sourceKeyName = logKeyName + L"\\" + sourceName;
        }
    };
}
