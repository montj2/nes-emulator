#include "../stdafx.h"

#include "../macros.h"
#include "framework.h"

#include <list>
using namespace std;

class TestFrameworkImpl
{
public:
	std::list<TestCase*> fTestCases;
};

TestFramework& TestCase::framework()
{
	return TestFramework::instance();
}

TestFramework* TestFramework::_singleton = NULL;

TestFramework::TestFramework()
{
	_pImpl = new TestFrameworkImpl();
}

TestFramework::~TestFramework()
{
	deleteAll();

	delete _pImpl;
	_pImpl = nullptr;
}

void TestFramework::addTestCase(TestCase *obj)
{
	_pImpl->fTestCases.push_back(obj);
}

template <class TC> TestResult TestFramework::runTestCase()
{
	TestCase* tcobj = new TC();
	auto result = runTestCase(tcobj);
	delete tcobj;
	return result;
}

TestResult TestFramework::runTestCase(TestCase *obj)
{
	printf("[-] runTestCase: %s\n", obj->name());
	// get ready
	puts("[ ] setting up...");
	obj->setUp();
	// run
	puts("[ ] running...");
	const TestResult result = obj->run();
	// verify result and print error message
	if (result != SUCCESS)
	{
		printf("[X] failed with code %d\n", result);
		obj->displayError();
	}
	// clean up
	puts("[ ] cleaning...");
	obj->tearDown();
	// done
	puts((result == SUCCESS) ? "[-] passed" : "[!] ended");
	puts("");
	return result;
}

void TestFramework::runAll()
{
	bool ok = true;
	puts("[+] runAll()");
	for (auto it : _pImpl->fTestCases)
	{
		auto result = runTestCase(it);
		if (result != SUCCESS)
		{
			ok = false;
		}
	}
	if (!ok)
	{
		puts("[+] not all unit tests passed. pls check.");
	}else
	{
		puts("[-] ALL TEST CASES PASSED!");
	}
}

void TestFramework::deleteAll()
{
	for (auto it : _pImpl->fTestCases)
	{
		delete it;
	}
	_pImpl->fTestCases.clear();
}