/**
 * @file app.cpp
 * @brief Plugin Main Implementation
 * @author Simon Steele
 * @note Copyright (c) 2006 Simon Steele <s.steele@pnotepad.org>
 *
 * Programmers Notepad 2 : The license file (license.[txt|html]) describes 
 * the conditions under which this source may be modified / distributed.
 */
#include "stdafx.h"
#include "sinks.h" 
#include "app.h"
#include "utils.h"
#include "wrapscintilla.h"

#if defined (_DEBUG)
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

typedef std::basic_string<TCHAR> tstring;

/**
 * Constructor
 */
App::App(boost::python::handle<>& obj, extensions::IPN* app) : m_app(app), main_module(obj)
{
	m_registry = m_app->GetScriptRegistry();
	if(m_registry)
	{
		m_registry->RegisterRunner("python", this);
		m_registry->EnableSchemeScripts("python", "python");
	}

	m_registry->Add("test", "Test Script", "python:testScript");

	main_namespace = main_module.attr("__dict__");
}

App::~App()
{
}

/**
 * Initialise the Python app environment
 */
void App::Initialise()
{
	// Before we do anything else, make sure python can find our files!
	// Find Me
	std::string s;
	m_app->GetOptionsManager()->GetPNPath(s);
	if((s[s.length()-1]) == '\\')
	{
		s.erase(s.length()-1);
	}

	// Run some Python...
	std::string setuppaths("import sys\nsys.path.append(r'" + s + "')\nsys.path.append(r'" + s + "\\scripts')\n\n");
	OutputDebugString(setuppaths.c_str());
	try
	{
		boost::python::handle<> ignored(PyRun_String(setuppaths.c_str(), Py_file_input, main_namespace.ptr(),main_namespace.ptr()));
	}
	catch(boost::python::error_already_set&)
	{
		std::string s = getPythonErrorString();
		OutputDebugString(s.c_str());
	}

	// Now run the init.py file
	loadInitScript();
}

void App::OnNewDocument(extensions::IDocumentPtr doc)
{
	extensions::IDocumentEventSinkPtr p(new DocSink(doc));
	doc->AddEventSink(p);
}

/**
 * Run a script that we registered
 */
void App::RunScript(const char* name)
{
	try
	{
		boost::python::call_method<void>(m_glue.ptr(), "runScript", name);
	}
	catch(boost::python::error_already_set&)
	{
		std::string s = getPythonErrorString();
		OutputDebugString(s.c_str());
	}
}

// Move the text back one character each time we find a CR
void fixCRLFLineEnds(char* buffer)
{
	bool sliding(false);
	char* pfixed(buffer);
	while(*buffer)
	{
		if(*buffer != 0x0D)
		{
			if(sliding)
				*pfixed = *buffer;
			pfixed++;
		}
		else
		{
			sliding = true;
		}
			
		buffer++;
	}
	*pfixed = NULL;
}

// Change the CRs to LFs
void fixCRLineEnds(char* buffer)
{
	while(*buffer)
	{
		if(*buffer == 0x0D)
		{
			*buffer = 0x0A;
		}
			
		buffer++;
	}
}

/**
 * Run an open document as if it was a script
 */
void App::RunDocScript(extensions::IDocumentPtr doc)
{
	HWND hWndScintilla = doc->GetScintillaHWND();
	if(hWndScintilla)
	{
		PNScintilla sc(hWndScintilla);
		
		int length = sc.GetLength();
		char* buffer = new char[length+1];
		sc.GetText(length+1, buffer);
		//buffer[length] = '\0';
		switch(sc.GetEOLMode())
		{
		case SC_EOL_CRLF:
			fixCRLFLineEnds(buffer);
			break;
		case SC_EOL_CR:
			fixCRLineEnds(buffer);
			break;
		}

		try
		{
			boost::python::handle<> ignored(PyRun_String(buffer,
				Py_file_input, 
				main_namespace.ptr(),
				main_namespace.ptr()
			));
		}
		catch(boost::python::error_already_set&)
		{
			std::string s = PyTracebackToString();
			OutputDebugString(s.c_str());
		}

		delete [] buffer;
	}
}

/**
 * Register a script with PN
 */
void App::RegisterScript(const char* scriptname, const char* group, const char* name)
{
	std::string scriptref("python:");
	scriptref += scriptname;
	m_registry->Add(group, name, scriptref.c_str());
}

extensions::IPN* App::GetPN() const
{
	return m_app;
}

boost::python::object& App::PyModule()
{
	return main_module;
}

boost::python::object& App::PyNamespace()
{
	return main_namespace;
}

boost::python::object& App::PyPnGlue()
{
	return m_glue;
}

/**
 * Load and run init.py
 */
void App::loadInitScript()
{
	extensions::IOptions* opts = m_app->GetOptionsManager();
	tstring path;
	opts->GetPNPath(path);

	TCHAR szpath[MAX_PATH+1];
	_tcscpy(szpath, path.c_str());

	::PathAppend(szpath, "init.py");

	runFile(szpath);

	m_glue = main_module.attr("glue");
}

/**
 * Load a file and run it through the python interpreter
 */
void App::runFile(const char* szpath)
{
	struct _stat statbuf;
	int result;
	result = _stat( szpath, &statbuf );
	if(result == 0)
	{
		FILE* init_script = fopen(szpath, "r");
		
		if(init_script != NULL)
		{
			std::string init;
			char* buf = new char[statbuf.st_size+1];
			while( fgets(buf, statbuf.st_size+1, init_script) )
			{
				init += buf;
			}
			fclose(init_script);

			try
			{
				boost::python::handle<> ignored(PyRun_String(init.c_str(),
					
					Py_file_input, 
					main_namespace.ptr(),
					main_namespace.ptr()
				));
			}
			catch(boost::python::error_already_set&)
			{
				std::string s = PyTracebackToString();
				OutputDebugString(s.c_str());
			}

			delete [] buf;
		}
	}
}