#ifndef x_IMACRO_UNITTEST_H_INCLUED_
#define x_IMACRO_UNITTEST_H_INCLUED_

#include "errlogger.h"
#include "executive.h"

namespace NIMacro {

	class Mod;
	class Module;
	class ModParTrier;

	/** Unit test system is meant for testing single modules, with automatically generated 
	* inputs, plus optionally with module-provided test case sets. The Mod class contains
	* supporting virtual member functions: CreateModParTrier(), PreCondition(), PostCondition(),
	* TestCases().
	*/
	class UnitTest: public mb_malloced, public ErrLogger {
	public:
		/// Create unit test for module m.
		UnitTest(Module& m);

		/** Run tests
		* @param exhaustive - exhaustive scan over all combinations of all parameters
		*/
		bool Run(bool exhaustive=false);

		/// Called from Mod::TestCases();
		bool TestCase(const char* test_name);

	private: // methods
		ModParTrier* ChooseModParTrier(int i);
		void RunModParTrials(bool exhaustive);
		void RunTestCases();
		void TestModuleParam(int fixed, bool exhaustive, int firstvaried);

	private:
		Module& m_;
		Mod* mod_;
		ModParTrier** triers_;
		PDataBlock db_;
		Executive exec_;
		ExecutionContext ctx_;
		Mod* input_;
	};

} // namespace
#endif
