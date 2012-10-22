#ifndef x_ACAPELLA_MEMBLOCK_STATISTIC_H_INCLUDED_
#define x_ACAPELLA_MEMBLOCK_STATISTIC_H_INCLUDED_

#include "vector.h"

namespace NIMacro {

struct StatisticImplBase;

/**
* A helper class for calculating numeric statistics on numeric arrays.
* Usage example:
* @code
*		PVector v = ...;
*		Statistic stat_object(v, "median", "exclude", "0.0");
*		for (int i=0; i<v->Length()-100; i+=100) {
*			double result = stat_object.Comp(i, 100);
*		}
* @endcode
*/
class DI_MemBlock Statistic {
public:
	/**
	* Construct a Statistic object for calculating a particular statistic on a particular vector.
	* @param data The data vector for preparing the input data.
	* @param statistic One of: "min", "max", "samplesize", "sum", "mean", "median", "quantile", "stddev", "variance".
	*				The "samplesize" statistic is nontrivial if inf_handling or nan_handling is set to "exclude".
	* @param nan_handling Either \"exclude\" for excluding NaN-s in the input data, \"include" for keeping them, or a numeric constant for replacing them.
	* @param inf_handling Either \"exclude\" for excluding inf-s in the input data, \"include" for keeping them, or a numeric constant for replacing them.
	* @param arg1 An additional parameter, specifies the quantile point for the quantile statistic.
	*/
	Statistic(const PVector data, const Nbaseutil::safestring& statistic, const Nbaseutil::safestring& nan_handling="exclude", const Nbaseutil::safestring& inf_handling="exclude", double arg1=0.0);

	/// Copy ctor
	Statistic(const Statistic& b);

	/// Assignment operator
	Statistic& operator=(const Statistic& b);

	/**
	* Calculate the statistic on a data array. This function is available for all native numeric datatypes.
	* @param pos Starting index in the vector.
	* @param length Length of the vector slice to include in the calculations. -1 means until the end of the vector.
	*/
	double Comp(int pos=0, int length=-1);

	~Statistic();

public: // implementation
	double GetNanReplace() const {return nan_replace_;}
	double GetInfReplace() const {return inf_replace_;}
	double GetArg1() const {return arg1_;}

private: // data
	StatisticImplBase* impl_;
	double nan_replace_, inf_replace_, arg1_;
	const PVector data_;
	const unsigned char* data_pointer_;
	int data_elemsize_, data_length_;
};


} // namespace
#endif
