#pragma once

#include <windows.h>

#include <fstream>
#include <string>

class CrashDiagnostics {
	std::ofstream log_file;
	bool enabled;

  public:
	CrashDiagnostics(const char* filename, bool enable = true)
		: enabled(enable) {
		if (enabled) {
			log_file.open(filename, std::ios::out | std::ios::trunc);
			if (log_file.is_open())
				Log("Crash diagnostics initialized");
		}
	}

	~CrashDiagnostics() {
		if (enabled && log_file.is_open()) {
			Log("Application terminating normally");
			log_file.close();
		}
	}

	void Log(const std::string& message) {
		if (!enabled || !log_file.is_open())
			return;

		SYSTEMTIME st;
		GetLocalTime(&st);

		char timestamp[64];
		sprintf_s(timestamp, sizeof(timestamp), "[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
				  st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

		log_file << timestamp << message << std::endl;
		log_file.flush();
	}

	void LogError(const std::string& context, HRESULT hr) {
		if (!enabled)
			return;

		char buffer[512];
		sprintf_s(buffer, sizeof(buffer), "ERROR in %s: HRESULT = 0x%08X (%d)", context.c_str(),
				  (unsigned int)hr, (int)hr);
		Log(buffer);
	}

	void LogException(const std::string& context, const char* what = nullptr) {
		if (!enabled)
			return;

		std::string message = "EXCEPTION in " + context;
		if (what)
			message += std::string(": ") + what;
		Log(message);
	}

	void LogStep(const std::string& step_name) {
		if (!enabled)
			return;

		Log(">>> " + step_name);
	}
};
