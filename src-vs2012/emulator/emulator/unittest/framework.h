#pragma once

class TestFramework;
class TestFrameworkImpl;

enum TestResult
{
	SUCCESS = 0,
	FAILED = 1
};

class TestCase
{
	friend class TestFramework;

public:
	virtual const char * name() = 0;

	// utility to retrieve the test framework
	static TestFramework& framework();

protected:
	virtual void setUp() {}
	
	// must be implemented by derived classes
	virtual TestResult run() = 0;

	virtual void tearDown() {}

	virtual void displayError() {}
};

template<class T>
class TestCaseAutoRegister
{
public:
	TestCaseAutoRegister()
	{
		TestFramework::instance().addTestCase(new T());
	}
};

class TestFramework
{
public:
	// test case manager
	template <class TC> TestResult runTestCase();
	TestResult runTestCase(TestCase *);
	void runAll();

	void addTestCase(TestCase *);
	void deleteAll();

	// unit test utility
	void assertion(const wchar_t *, const wchar_t *, unsigned long, TestCase * tc = nullptr);

	// singleton
	static inline TestFramework& instance()
	{
		// lazy-load
		if (_singleton == nullptr)
			_singleton = new TestFramework();

		return *_singleton;
	}

	static void destroy()
	{
		if (_singleton != nullptr)
		{
			delete _singleton;
			_singleton = NULL;
		}
	}

private:
	TestFramework();
	~TestFramework();
	TestFramework(const TestFramework&);

	static TestFramework *_singleton;

	TestFrameworkImpl *_pImpl;
};

#define registerTestCase(C) static TestCaseAutoRegister<C> __ ## C ## _register

#define tassert(E) if (!(E)) (framework().assertion(_CRT_WIDE(#E), _CRT_WIDE(__FILE__), __LINE__, this))