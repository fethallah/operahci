//#############################################################################################
//# Acapella Script and procedures Collection: A collection of Acapella based scripts
//# and libraries.
//#
//# Please acknowledge the author(s)/contributor(s) for making use of these scripts and library.
//# Authors:
//#   * Ghislain M.C. Bonamy
//#   * Genomics Institute of the Novartis Research Foundation (GNF), San Diego, CA 92122
//#   * www.gnf.org
//#
//# You can obtain the latest version of this software at: http://code.google.com/p/operahci/
//#
//# This program is free software; you can redistribute it and/or modify
//# it under the terms of the GNU General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or
//# (at your option) any later version.
//#
//# This program is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License
//# along with this program; if not, write to the Free Software
//# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//# or go to: http://www.apache.org/licenses/
//#############################################################################################

//*********************************************************************************************
//*
//* This library provide new statistical features
//*  - CalcCorelation returns the corelation of 2 vectors.
//*  - pairWiseDistance, calculates the distance between each objects provided from one another
//*
//*********************************************************************************************


#include <acapella/DI_acapella.h>
#include <algorithm>
#include <memblock/pmemblock.h>
#include <math.h>
#include <cells/DI_cells.h>
#include <algorithm>


using namespace NIMacro;

class CalcCorrelation: public Mod {
public: // interface
	static Mod* Create() {return new CalcCorrelation();}
private: // implementation
	CalcCorrelation() {}
	virtual void Declare();
	virtual void Run();
private: // module parameters
	safestring attribute1;
	safestring attribute2;
	//const PMemBlock objects;
	const PCells objects;
	double COV;
	double R;
};

void CalcCorrelation::Declare() {
	// Declare module groups and general description
	module(0, "statistics, GNF_StatisticLibrary", "Returns the covariance (COV) and Pearson's correlation coefficient (R) of two array from the provided objects");
	// Declare module input parameters
	input(attribute1, "attribute1", PAR_POSITIONAL|PAR_NOQUOTE, "Name of the first attribute containing the vector to use.");
	input(attribute2, "attribute2", PAR_POSITIONAL|PAR_NOQUOTE, "Name of the second attribute containing the vector to correlate.");
	input(objects, "Objects (objectlist)",PAR_POSITIONAL|PAR_NOQUOTE, "Objects containing the vectors to use");
	// Declare module output parameters
	output(COV, "COV", 0, "Covariance between the two vectors provided.");
	output(R, "R", 0, "Pearson Correlation Coefficient for the two vectors provided.");
}


// Algorithm for the function CalcCorrel()
// Adapted from: http://en.wikipedia.org/wiki/Correlation_coefficient
void CalcCorrelation::Run() {


	const PVector x = objects->GetSubItemVector(attribute1,DoNothing);
	if(x==NULL)Throw(Exception(ERR_BADPARAMETERS,"Objects does not contain Attribute1="+ attribute1+". Objects is an "+objects->ToString()),ThrowIndeed);
	const PVector y = objects->GetSubItemVector(attribute2,DoNothing);
	if(x==NULL)Throw(Exception(ERR_BADPARAMETERS,"Objects does not contain Attribute1="+ attribute2+". Objects is an "+objects->ToString()),ThrowIndeed);

	x=x->ConvertElemsToType(Vector::Double,true,true);
	y=y->ConvertElemsToType(Vector::Double,true,true);

	if( x->Length() < 2) {
		throw NIMacro::Exception(ERR_BADPARAMETERS,"The attribute vectors should have more than one value.");
		//Abort();
	}
	if( y->Length() < 2) {
		throw NIMacro::Exception(ERR_BADPARAMETERS,"The attribute vectors should have more than one value.");
		//Abort();
	}



	double size=0;
	safestring name;
	if(x->Length() !=  y->Length()) {
		if(x->Length()<y->Length()){
			size=(double) x->Length();
			name=attribute1;
		}
		else {
			size=(double) y->Length();
			name=attribute2;
		}
		Warning(WARN_SUSPICIOUS_PARAMETERS,"The two attributes vectors should have the same lenght. Correlation and covariance calculated using the attribute with the fewest number of elements: "+name);
	}else{
		size=(double) x->Length();
	}

	double sweep=0 ,delta_x=0,delta_y=0,sum_sq_x=0,sum_sq_y=0,sum_coproduct=0, mean_x=0, mean_y=0;
	const double* valX = x->DoublePointer();
	const double* valY= y->DoublePointer();
	const double factorX= x->Factor();
	const double factorY=y->Factor();

	mean_x	  =  factorX*valX[0];
	mean_y	  =  factorY*valY[0];

	for(double i = 1; i < size; i++){
		sweep =  i / (i + 1.0);
		delta_x =  factorX*valX[(int)i] - mean_x; 
		delta_y =  factorY*valY[(int)i] - mean_y;
		sum_sq_x =  sum_sq_x + delta_x * delta_x * sweep;
		sum_sq_y =  sum_sq_y + delta_y * delta_y * sweep;
		sum_coproduct  =  sum_coproduct + delta_x * delta_y * sweep;
		mean_x =  mean_x + delta_x / (i+1.0);
		mean_y =  mean_y + delta_y / (i+1.0);
	}
	double pop_sd_x=  sqrt (sum_sq_x / size);
	double pop_sd_y=  sqrt (sum_sq_y / size );
	COV=  sum_coproduct / size;
	R=  COV / (pop_sd_x * pop_sd_y);
}

class BinData: public Mod {
public: // interface
	static Mod* Create() {return new BinData();}
	static PVector sortNumberVector(PVector values);
private: // implementation
	BinData() {}
	virtual void Declare();
	virtual void Run();
	
	PVector cummultatedBinning(PVector data, PVector boundaries);
private: // module parameters
	const PVector data;
	const PVector boundaries;
	PVector biningData;
	PVector boundariesName;
};


void BinData::Declare() {
	// Declare module groups and general description
	module(0, "statistics, GNF_StatisticLibrary", "Returns a vector containing the binning boundaries and one containnig the binned data");
	// Declare module input parameters
	input(data, "data (vector: integer, double)", PAR_POSITIONAL|PAR_NOQUOTE, "Vector containing the data to bin.");
	input(boundaries, "Boundaries (vector: int, unsigned int, double, float)", PAR_POSITIONAL|PAR_NOQUOTE, "Vector containing the boundaries to use.");
	// Declare module output parameters
	output(biningData, "Data (vector: unsigned int)", 0, "Binned data returned");
	output(boundariesName, "Boundaries (vector: string)", 0, "Boundaries used formated.");
}

void BinData::Run() {
	/*if (strcmp(data->Class(), "vector")!=0) {
		throw NIMacro::Exception(ERR_BADPARAMETERS,"\"Data\" must be a vector");
	}
	if (strcmp(boundaries->Class(), "vector")!=0) {
		throw NIMacro::Exception(ERR_BADPARAMETERS,"\"Data\" must be a vector");
	}
	*/

	if(boundaries->Length()<1){
		throw NIMacro::Exception(ERR_BADPARAMETERS,"The vector containing the boundaries should have more than one value.");
	}
	PVector dataClone = data->Clone();
	PVector boundariesClone= boundaries->Clone();

	


	Vector::DataType type;
	type = dataClone->ElemType();
	if (type!=Vector::Int && type!=Vector::Double && type!=Vector::Float && type!=Vector::UnsignedInt) 
		throw NIMacro::Exception(ERR_BADPARAMETERS,"\"Data\" must be a vector of numbers (Int, Unsigned int, Float or Double).");
	type = boundariesClone->ElemType();
	if(type!=Vector::Int && type!=Vector::Double && type!=Vector::Float && type!=Vector::UnsignedInt) 
		throw NIMacro::Exception(ERR_BADPARAMETERS,"\"Boundaries\" must be a vector of numbers (Int, Unsigned int, Float or Double).");

	boundariesClone=BinData::sortNumberVector(boundariesClone);
	boundariesName =  Vector::Create(boundariesClone->Length()+1,Vector::String);
	safestring* boundariesNameValues = boundariesName->StringPointer();
	

	safestring start = "X<=";
	safestring end = "<X";
	safestring mid= "<=X<";
	safestring dataTypePrint = boundariesClone->ElemType()==Vector::Int||boundariesClone->ElemType()==Vector::UnsignedInt?"%d":"%4.2g";
	boundariesClone=boundariesClone->ConvertElemsToType(Vector::Double);
	double* boundariesCloneValuesDbl=boundariesClone->DoublePointer();


	boundariesNameValues[0]=start;
	boundariesNameValues[0]+=Printf(dataTypePrint)(boundariesCloneValuesDbl[0]);


	boundariesNameValues[boundaries->Length()]=Printf(dataTypePrint)(boundariesCloneValuesDbl[boundaries->Length()-1]);
	boundariesNameValues[boundaries->Length()]+=end;

	for(int i=1; i<(int) boundaries->Length();i++){
		boundariesNameValues[i]=Printf(dataTypePrint)(boundariesCloneValuesDbl[i-1]);
		boundariesNameValues[i]+=mid;
		boundariesNameValues[i]+=Printf(dataTypePrint)(boundariesCloneValuesDbl[i]);
	}

	


	biningData = Vector::Create(boundariesClone->Length()+1,Vector::UnsignedInt);
	if(dataClone->Length()<1){
		Warning(WARN_SUSPICIOUS_PARAMETERS,"\"data\" does not contain any elements, the binning data values returned are all 0");
		return;
	}
	//dataClone=dataClone->ConvertElemsToType(Vector::Double);
	dataClone=BinData::sortNumberVector(dataClone);	
	boundariesClone = BinData::cummultatedBinning(dataClone, boundariesClone);

	int* boundariesCloneValues=boundariesClone->IntPointer();
	int* biningDataValues=biningData->IntPointer();
	biningDataValues[0]=boundariesCloneValues[0];
	biningDataValues[boundaries->Length()]=dataClone->Length()-boundariesCloneValues[boundaries->Length()-1];
	for(int i=1;i<(int) boundaries->Length();i++){
		biningDataValues[i]=boundariesCloneValues[i]-boundariesCloneValues[i-1];
	}	
}

PVector BinData::cummultatedBinning(PVector data, PVector boundaries){

	boundaries= boundaries->ConvertElemsToType(Vector::Double);
	double* boundariesValues = boundaries->DoublePointer();	
	PVector cummultatedData = Vector::Create(boundaries->Length(),Vector::Int);
	int* cummulatedValues = cummultatedData->IntPointer();
	

	data=data->ConvertElemsToType(Vector::Double);
	double* dataValues = data->DoublePointer();
	int index=0;
	for(int i = 0; i < (int) boundaries->Length(); i++){
		cummulatedValues[i]=data->Length(); //if no values of Data is superior to boundariesValues[i], the cummulated value is the total number of elements in the vector (next condition never satisfied)
		for(int j = index;j<(int) data->Length();j++){
			if(boundariesValues[i]*boundaries->Factor()<dataValues[j]*data->Factor()){
				cummulatedValues[i]=j; //use the previous index which is <= to that of dataValues[j], index+1 is the cummulated value bellow the boundary
				index=j;
				break;
			}
			if(i==(int) boundaries->Length()-1){
				index=boundaries->Length(); //no more values in the data are uperior to boundariesValues[i], stop iterating of data
			}
		}
	}
	return cummultatedData;
}

PVector BinData::sortNumberVector(PVector values){
		Vector::DataType type=values->ElemType();
		switch(values->ElemType()){
		case Vector::Int:
			std::sort(values->IntPointer(),values->IntPointer()+values->Length());
			break;	
		case Vector::UnsignedInt:
			std::sort(values->UnsignedIntPointer(),values->UnsignedIntPointer()+values->Length());
			break;
		case Vector::Double:
			std::sort(values->DoublePointer(),values->DoublePointer()+values->Length());
			break;
		case Vector::Float:
			std::sort(values->FloatPointer(),values->FloatPointer()+values->Length());
			break;
		default:
			throw NIMacro::Exception(ERR_BADPARAMETERS,"vector is not supported for sorting. Supported types are vectors of numbers: (Int, Unsigned int, Float or Double).");
		}
		return values;
	}


class FindPercentileValues: public Mod {
public: // interface
	static Mod* Create() {return new FindPercentileValues();}
private: // implementation
	FindPercentileValues() {}
	virtual void Declare();
	virtual void Run();
	PVector sortNumberVector(PVector values);
	PVector getBoundaries(PVector data, PVector boundaries);
private: // module parameters
	const PVector data;
	PVector boundaries;
	PVector percentileData;
};

void FindPercentileValues::Declare() {
	// Declare module groups and general description
	module(0, "statistics, GNF_StatisticLibrary", "Returns a vector containing the percentile boundaries formated  and one containnig the binned data");
	// Declare module input parameters
	input(data, "data (vector: integer, double)", PAR_POSITIONAL|PAR_NOQUOTE, "Vector containing the data to bin.");
	input(boundaries, "PercentilesBoundaries (vector: int, unsigned int, double, float)", PAR_POSITIONAL|PAR_NULLDEFAULT|PAR_NOQUOTE, "Vector containing the percentile boundaries to use. Provide values as percentage (ex. 1 = 1%) If no values is provided, use the percentile caracteristic of the normal distribution distribution boundaries (0.1%,2.2%,15.8%,50%,etc.)");
	// Declare module output parameters
	output(percentileData, "PercentilesCutOff (vector: int, double, float)", 0, "Data containing corresponding to a given ==.");
	output(boundaries, "PercentilesBoundaries (vector: double)", 0, "Formated percentile values used for the boundaries.");
}

void FindPercentileValues::Run() {
	/*if (strcmp(data->Class(), "vector")!=0) {
		throw NIMacro::Exception(ERR_BADPARAMETERS,"\"Data\" must be a vector");
	}
	if (strcmp(boundaries->Class(), "vector")!=0) {
		throw NIMacro::Exception(ERR_BADPARAMETERS,"\"Data\" must be a vector");
	}
	*/

	PVector dataClone = data->Clone();
	PVector boundariesClone;

	if(boundaries!=NULL){
		boundariesClone = boundaries->Clone();
	}
	else{
		double array[] = {0.1, 2.2, 15.8, 50, 84.2, 97.8, 99.9};
		boundariesClone = Vector::Create(sizeof(array) /sizeof(array[0]) ,Vector::Double);
		std::copy(array, array + sizeof(array)/sizeof(array[0]), boundariesClone->DoublePointer());
	}
	
	if(boundariesClone->Length()<1){
		throw NIMacro::Exception(ERR_BADPARAMETERS,"The vector containing the boundaries should have more than one value.");
	}

	Vector::DataType type= dataClone->ElemType();
	if (type!=Vector::Int && type!=Vector::Double && type!=Vector::Float && type!=Vector::UnsignedInt) 
		throw NIMacro::Exception(ERR_BADPARAMETERS,"\"Data\" must be a vector of numbers (Int, Unsigned int, Float or Double).");
	type = boundariesClone->ElemType();
	if(type!=Vector::Int && type!=Vector::Double && type!=Vector::Float && type!=Vector::UnsignedInt) 
		throw NIMacro::Exception(ERR_BADPARAMETERS,"\"Percentiles\" must be a vector of numbers (Int, Unsigned int, Float or Double).");


	boundariesClone=BinData::sortNumberVector(boundariesClone);
	


	percentileData = Vector::Create(boundariesClone->Length(),Vector::UnsignedInt);
	if(dataClone->Length()<1){
		Warning(WARN_SUSPICIOUS_PARAMETERS,"\"data\" does not contain any elements, the binning data values returned are all 0");
		return;
	}
	dataClone=BinData::sortNumberVector(dataClone);	

	percentileData = FindPercentileValues::getBoundaries(dataClone, boundariesClone);
	//percentileData=percentileData->ConvertElemsToType(data->ElemType());
	boundaries=boundariesClone;
}

PVector FindPercentileValues::getBoundaries(PVector data, PVector boundaries){


	boundaries= boundaries->ConvertElemsToType(Vector::Double);
	double* boundariesValues = boundaries->DoublePointer();	
	PVector boundariesData = Vector::Create(boundaries->Length(),Vector::Double);
	double* boundariesDataValues = boundariesData->DoublePointer();

	if(boundariesValues[0]<0)
		throw NIMacro::Exception(ERR_BADPARAMETERS,"The values contained in the vector \"Percentiles\" must be bellow or equals to 100 (100%).");
	if(boundariesValues[boundaries->Length()-1]>100)
		throw NIMacro::Exception(ERR_BADPARAMETERS,"The values contained in the vector \"Percentiles\" must be above or equal to 0 (0%).");

	data=data->ConvertElemsToType(Vector::Double);
	double* dataValues=data->DoublePointer();


	for(int i = 0; i < (int) boundaries->Length(); i++){
		boundariesDataValues[i]= (unsigned int)(((double)data->Length() * boundariesValues[i]/100.0) - 0.5); //round to the closest value index+1 is the cummulated value bellow the boundary
	}

	bool begining=true;
	for(int i = 0; i < (int) boundaries->Length()-1; i++){
		if(begining && boundariesDataValues[i]==boundariesDataValues[i+1])
			boundariesDataValues[i]= std::numeric_limits<double>::quiet_NaN(); //set the value to Not Applicable
		else if	(!begining && (boundariesDataValues[i]==boundariesDataValues[i+1]||boundariesDataValues[i]!=boundariesDataValues[i])) //check if the value is the same or already set to one (if f=NaN, then f!=f)
			boundariesDataValues[i+1]= std::numeric_limits<double>::quiet_NaN(); //set the value to Not Applicable
		else
			boundariesDataValues[i]=dataValues[(int) boundariesDataValues[i]];
	}
	if	(boundariesDataValues[boundaries->Length()-1]==boundariesDataValues[boundaries->Length()-1])
	boundariesDataValues[boundaries->Length()-1]=dataValues[(int) boundariesDataValues[boundaries->Length()-1]];
	boundariesData->SetFactor(data->Factor());
	return boundariesData;
}

class pairWiseDistance: public Mod {
public: // interface
	static Mod* Create() {return new pairWiseDistance();}
private: // implementation
	pairWiseDistance() {}
	virtual void Declare();
	virtual void Run();
private: // module parameters
	const PVector xCoord;
	const PVector yCoord;
	PVector distances;
	PVector shortestDistance;
};

void pairWiseDistance::Declare() {
	// Declare module groups and general description
	module(0, "statistics, GNF_StatisticLibrary", "Returns a vector of vectors containing the pairwise distance of a set of points");
	// Declare module input parameters
	input(xCoord, "X (vector: double)", PAR_POSITIONAL, "Vector containing the X-coordinates for the set of points.");
	input(yCoord, "Y (vector: double)", PAR_POSITIONAL, "Vector containing the Y-coordinates for the set of points.");
	// Declare module output parameters
	output(distances, "Distances (vector: double)", 0, "Data containing the vector of distances between each points to the rest of the points.");
	output(shortestDistance, "shortestDistance (vector: double)", 0, "Data providing the distance to the nearest point.");
}

void pairWiseDistance::Run() {
	
	if(xCoord->Length()!=yCoord->Length())
		throw NIMacro::Exception(ERR_BADPARAMETERS,"\"X\" and \"Y\" must have the same length.");
	const double* xCoordVal=xCoord->DoublePointer();
	const double* yCoordVal=yCoord->DoublePointer();

	int size = (int) xCoord->Length();
	distances=Vector::Create((size*(size-1))/2, Vector::Double);
	shortestDistance=Vector::Create(size,Vector::Double);
	double* distancesValues = distances->DoublePointer();
	double* shortestDistanceValues = shortestDistance->DoublePointer();
	double x=0, y=0, xCoordFact=xCoord->Factor(), yCoordFact=yCoord->Factor();
	double shortestDist=0,currentVal=0;
	int shortestDistIndex=0;
	int index=1, fillIndex=0;
	for(int i=0;i < size-1;i++){
		shortestDist=std::numeric_limits<double>::max();
		for(int j=index;j < size;j++){
			x=(xCoordVal[i]-xCoordVal[j])*xCoordFact;
			x*=x;
			y=(yCoordVal[i]-yCoordVal[j])*yCoordFact;
			y*=y;
			currentVal=sqrt(x+y);
			distancesValues[fillIndex]=currentVal;
			if(shortestDist>currentVal){
				shortestDist=currentVal;
				shortestDistIndex=j;
			}

			if(shortestDistanceValues[i]==0){ //only modify values that have not been set yet
				shortestDistanceValues[i]=shortestDist;
				shortestDistanceValues[shortestDistIndex]=shortestDist;	
			}
			fillIndex++;
		}
		index++;
	}
}


// Module export
extern "C"
#ifdef _MSC_VER
__declspec(dllexport)
#endif
int ExportedModules( ModReg** x, int *Count) {
	static ModReg p[] = {
		// Declare module names and creation function addresses
		{"CalcCorrelation", CalcCorrelation::Create},
		{"BinData", BinData::Create},
		{"FindPercentileValues", FindPercentileValues::Create},
		{"PairWiseDistance", pairWiseDistance::Create},
	};
	*x = p;
	*Count = sizeof(p)/sizeof(ModReg);
	return MOD_CLASS_LIBRARY;
}
