
// A class for holding tabular data.
// First column (named "data" by default) is handled and accessible by Vector base class.
// Additional columns can be added freely. 
// All columns must have the same length.

#ifndef _TABLE_H_INCLUDED_
#define _TABLE_H_INCLUDED_

#include <vector>


#include "vector.h"
#include "container.h"


namespace NIMacro {
class DI_MemBlock Table;
typedef DerivedPointer<Table> PTable;

/**
* Class for representating tables. Table has zero or more columns. 
* Each column conforms to a Vector and has a non-empty unique name.
* All columns are of the same length but the data type of columns may differ.
*/
// Change 26.10.2009: derive Table from Container, deriving from a vector-of-vectors makes no sense.
class DI_MemBlock Table: public Container {
	typedef Container super;
public:
	// Addition 13.08.2003:
	/// Create an empty table whose all columns are of the same type. Throw in case of errors.
	static PTable Create(int columns, int rows, Vector::DataType t); 

	// Change 13.08.2003: Demand explicit name for the first column.
	/// Create a table and insert 1-3 existing columns. Throw in case of errors.
	static PTable Create(const Nbaseutil::safestring& name1, const PVector poData1, const Nbaseutil::safestring& name2="", PVector poData2=NULL, const Nbaseutil::safestring& name3="", PVector poData3=NULL); 

	/**
	* Replace or add a column. Throw in case of errors. The name must be a simple alphanumeric name, optionally containing position specifiers (:\@first, :\@last, :\@before:xxx, :\@after:xxx).
	* @return 0 - return type is int for binary back-compatibility, bu the return value is always 0 and not useful.
	*/
	int SetColumn(const Nbaseutil::safestring& name, PVector poColumn);

	/// Replace column. Throws if the column cannot be replaced:
	void ReplaceColumn(int colindex, const Nbaseutil::safestring& newname, PVector column);

	/// Changes the positions of columns specified by the arguments. The indices must be in range [0..ColumnCount()-1].
	void SwapColumns(int colindex1, int colindex2);

	/// Deletes a column. The indices of later columns wil change. Nothing happens if the column is not found.
	void DeleteColumn(const Nbaseutil::safestring& name);

	/// Deletes a column. The indices of later columns will change. Nothing happens if the column is not found.
	void DeleteColumn(int colindex);

	/**
	* Appends a row from the values in the container. If data is missing for a column or 
	* is not convertible to the column format, an empty/zero element is inserted.
	* @param data The source data container.
	* @param create_new_columns If the container contains new keys, create corresponding new columns.
	* @param init_missing_floats_to_nan If the container does not contain values for float or double columns, insert NaN values in the appended row, otherwise they remain 0.0.
	*/
	void AppendRow(const Container& data, bool create_new_columns=true, bool init_missing_floats_to_nan=true);

	/**
	* Appends rows from another table. If data is missing for a column or 
	* is not convertible to the column format, an empty/zero element is inserted.
	* @param data The source data table.
	* @param create_new_columns If the appended table contains new columns, create corresponding new columns in this table.
	*/
	void AppendRows(const Table& data, bool create_new_columns=true);

	/// Returns true if the table does not contain any data. One can insert a column of any length into such a table. 
	bool IsEmpty() const {return ColumnCount()==0;}

	/// Returns the number of columns, or 0 if the table does not contain any data.
	int ColumnCount() const;	

	/// Return the number of rows. For an empty table returns 0. Use IsEmpty() to determine if the table contains no data.
	int RowCount() const {return rowcount_;}

	/// Return a column, or NULL if column not found.
	PVector Column(const Nbaseutil::safestring& name) const;

	/// Return a column, or NULL if column not found.
	PVector Column(int iNo) const;

	/// Return column name, or an empty string, if column not found.
	const Nbaseutil::safestring& ColumnName(int iNo) const;

	/// Return column index, or -1, if column not found.
	int ColumnIndex(const Nbaseutil::safestring& colname) const;
	
	Nbaseutil::safestring GetDataName() const {return ColumnName(0);} // Included for backward compatibility.

	void SetDataName(const Nbaseutil::safestring& DataName) {	// Included for backward compatibility.
		if (ColumnCount()>0) {
			ReplaceColumn(0, DataName, Column(0));
		}
	}

	/// Return row content packed as a container. Returns NULL if rowindex out of range.
	PContainer GetRow(int rowindex) const;

	// Helper functions
	/// Return a default name A, B, .., Z, AA, AB, etc. for column index 0,1,2,..
	static Nbaseutil::safestring DefaultColName(int k);

	static PTable Create();

	// Overriden MemBlock virtual methods:
	virtual const char*		Class() const {return "Table";}	
	virtual PMemBlock		DoClone(copymode_t copy_mode) const;
	virtual SafeValue		DoVerb(const char* pszCommand, int iArg, SafeValue vArg[]);
	virtual bool			Conforms(const char *szClassName) const;
	virtual bool			Consistent(Nbaseutil::safestring& msg, bool CheckContent=true) const;	
	virtual PMemBlock		ConvertTo(const Nbaseutil::safestring& classname) const;
	virtual bool			Entertain(MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode=enter_deep);	
	virtual void			IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
	virtual SafeValue		ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed) const;	
protected:
	virtual bool			DoSetSubItem(const Nbaseutil::safestring& subitemname, const SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=ThrowIndeed);
	virtual Nbaseutil::safestring DoGetDescription() const;
	virtual void DoSerialize(int ser_format, SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const Formatter& formatter, Nbaseutil::safestring& buffer) const;
	virtual void Put(const SafeValue& item, const Nbaseutil::safestring& label);

public:
	// Overriden Vector virtual functions
	virtual void Erase(int pos, int n);	// erase up to n elements starting at pos.

protected:
	virtual bool Equals(const MemBlock& b) const; // value compare to another item b, which must be of the same dynamic type. See operator==()

	/// Verify that the column can be inserted into the table.
	void CheckColumn(const Nbaseutil::safestring& name, PVector& column) const;

private: // data
	int rowcount_;

	// Not to be used - function not implemented
	int Length(int dummy) const; 

protected:
	// Constructor for specific number of columns.
	Table(int columns);
};
} // namespace NIMacro
#endif
