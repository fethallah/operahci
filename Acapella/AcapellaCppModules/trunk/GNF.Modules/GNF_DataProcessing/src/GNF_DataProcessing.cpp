//#############################################################################################
//# Acapella Script and procedures Collection: A collection of Acapella based scripts
//# and libraries.
//#
//# Please acknowledge the author(s)/contributor(s) for making use of these scripts and library.
//# Authors:
//# * Ghislain M.C. Bonamy
//# * Genomics Institute of the Novartis Research Foundation (GNF), San Diego, CA 92122
//# * www.gnf.org
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
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License
//# along with this program; if not, write to the Free Software
//# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111-1307USA
//# or go to: http://www.apache.org/licenses/
//#############################################################################################

//*********************************************************************************************
//*
//* This library provides tools for data processing:
//*- ExtractROI returns the ROI as a table of Chain codes
//*
//*********************************************************************************************

#include <acapella/DI_acapella.h>
#include <algorithm>
#include <memblock/pmemblock.h>
#include <math.h>
#include <algorithm>
#include <sstream>

using namespace NIMacro;

class ExtractROI: public Mod {
public:
	// interface
	static Mod* Create() {
		return new ExtractROI();
	}
private:
	// implementation
	ExtractROI() {
	}
	virtual void Declare();
	virtual void Run();

	void polygonize(PVector coordinatesX, PVector coordinatesY,
			unsigned int& boundingBoxMinX, unsigned int& boundingBoxMinY,
			unsigned int& boundingBoxMaxX, unsigned int& boundingBoxMaxY,
			unsigned int& iniX, unsigned int& iniY, PVector chainCode, std::stringstream& chainCodeStr );
private:
	// module parameters
	const PIntervalVector stencil;
	bool chainCodeAsString;
	PTable table;
};

void ExtractROI::Declare() {
	// Declare module groups and general description
	module(0, "Object list manipulations, GNF_Dataprocessing",
			"Returns a table containing information about the different ROI (Region of Interest) encoded in the Stencil.\n"
					"The ROI path is encoded as a chain code which require a start point (cf. iniX and iniY) and then can be traced by following the chain "
					"of next moves encoded by the following array of vector (encoding matrix):<br>"
					"{ { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 },{ -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } }<br>"
					"The Chain code is counter clockwise, in this case a value of  0 moves the vector by x+1 and y+0<br>"
					"Vectors are reported in the encoding matrix as \"{X,Y}\". The coordinate system is: (X=0;Y=0) corresponds to the top left of the image "
					"and Y increases going down, X increases going up. This follows Acapella nomenclature.");
	// Declare module input parameters
	input(stencil, "Stencil (intervalvector)", PAR_POSITIONAL,
			"Stencil containing the border of the ROI to extract.");
		// Declare module input parameters
	input(chainCodeAsString, "chainCodeAsString (boolean)", PAR_BOOLEAN,
			"Indicates if you wish to have the chain code returned as a vector or as a string (default: true).");
	// Declare module output parameters

	output(table, "table (table)", 0,
			"Table containing the information about various properties of the object. The following columns are:"
					"<li>'BoundingBoxMinX': Contains the X coordinates for bottom left corner of each ROI bounding box."
					"<li>'BoundingBoxMinY': Contains the Y coordinates for bottom left corner of each ROI bounding box."
					"<li>'BoundingBoxMaxX': Contains the X coordinates for top right corner of each ROI bounding box."
					"<li>'BoundingBoxMaxY': Contains the Y coordinates for top right corner of each ROI bounding box."
					"<li>'IniX': Contains the X coordinates for the starting point of the chain code encoding the ROI."
					"<li>'IniY': Contains the Y coordinates for the starting point of the chain code encoding the ROI."
					"<li>'ChainCode': Contains a vector of vector or vector of strings (depending on 'chainCodeAsString') representing the ROI shape.");
}

void ExtractROI::Run() {

	const int* inner, *outer;
	unpack_two_layer_const(stencil, inner, outer);
	int objectNum = stencil->IntervalCount();

	PVector boundingBoxMinXList = Vector::Create(objectNum,
			Vector::UnsignedInt);
	unsigned int* boundingBoxMinXListValue =
			boundingBoxMinXList->UnsignedIntPointer();
	PVector boundingBoxMinYList = Vector::Create(objectNum,
			Vector::UnsignedInt);
	unsigned int* boundingBoxMinYListValue =
			boundingBoxMinYList->UnsignedIntPointer();

	PVector boundingBoxMaxXList = Vector::Create(objectNum,
			Vector::UnsignedInt);
	unsigned int* boundingBoxMaxXListValue =
			boundingBoxMaxXList->UnsignedIntPointer();
	PVector boundingBoxMaxYList = Vector::Create(objectNum,
			Vector::UnsignedInt);
	unsigned int* boundingBoxMaxYListValue =
			boundingBoxMaxYList->UnsignedIntPointer();

	PVector iniXList = Vector::Create(objectNum, Vector::UnsignedInt);
	unsigned int* iniXListValue = iniXList->UnsignedIntPointer();
	PVector iniYList = Vector::Create(objectNum, Vector::UnsignedInt);
	unsigned int* iniYListValue = iniYList->UnsignedIntPointer();

	PVector chainCodeList = Vector::Create(objectNum, Vector::PMemory);
	PMemBlock* chainCodeListValue = chainCodeList->PMemoryPointer();

	PVector chainCodeStrList = Vector::Create(objectNum, Vector::String);
	safestring* chainCodeStrListValue = chainCodeStrList->StringPointer();

	table = Table::Create();

	unsigned int width, height;
	stencil->GetImageDimensions(width, height);
	// i iterates over objects 1..n. i is an index into the outer array
	for (int i = 1; i <= objectNum; ++i) {
		// j is an index into the inner array, it iterates over object pixels in the inner array.
		PVector coordinatesX = Vector::Create(outer[i + 1] - outer[i],
				Vector::Int);
		int* coordinatesXValues = coordinatesX->IntPointer();
		PVector coordinatesY = Vector::Create(outer[i + 1] - outer[i],
				Vector::Int);
		int* coordinatesYValues = coordinatesY->IntPointer();

		int index = 0;
		for (int j = outer[i]; j < outer[i + 1]; ++j) {
			// inner[j] is the pixel index in the image, convert it to x and y coordinates:
			coordinatesXValues[index] = inner[j] % width;
			coordinatesYValues[index] = inner[j] / width;
			index++;
		}
		unsigned int boundingBoxMinX, boundingBoxMinY, boundingBoxMaxX,
				boundingBoxMaxY, iniX, iniY;
		PVector chainCode = Vector::Create(0, Vector::UnsignedInt);
		std::stringstream chainCodeStr;

		ExtractROI::polygonize(coordinatesX, coordinatesY, boundingBoxMinX,
				boundingBoxMinY, boundingBoxMaxX, boundingBoxMaxY, iniX, iniY,
				chainCode, chainCodeStr);
		boundingBoxMinXListValue[i - 1] = boundingBoxMinX;
		boundingBoxMinYListValue[i - 1] = boundingBoxMinY;
		boundingBoxMaxXListValue[i - 1] = boundingBoxMaxX;
		boundingBoxMaxYListValue[i - 1] = boundingBoxMaxY;
		iniXListValue[i - 1] = iniX;
		iniYListValue[i - 1] = iniY;
		chainCodeListValue[i - 1] = chainCode;
		chainCodeStrListValue[i - 1] = Printf("%s")(chainCodeStr.str());

	}
	table->SetColumn("BoundingBoxMinX", boundingBoxMinXList);
	table->SetColumn("BoundingBoxMinY", boundingBoxMinYList);
	table->SetColumn("BoundingBoxMaxX", boundingBoxMaxXList);
	table->SetColumn("BoundingBoxMaxY", boundingBoxMaxYList);
	table->SetColumn("IniX", iniXList);
	table->SetColumn("IniY", iniYList);
	if(chainCodeAsString)
		table->SetColumn("ChainCode", chainCodeStrList);
	else
		table->SetColumn("ChainCode", chainCodeList);
}

void ExtractROI::polygonize(PVector coordinatesX, PVector coordinatesY,
		unsigned int& boundingBoxMinX, unsigned int& boundingBoxMinY,
		unsigned int& boundingBoxMaxX, unsigned int& boundingBoxMaxY,
		unsigned int& iniX, unsigned int& iniY, PVector chainCode, std::stringstream& chainCodeStr) {

	int* coordinatesXValues = coordinatesX->IntPointer();
	int* coordinatesYValues = coordinatesY->IntPointer();

	int width = 0, height = 0, xMax = 0, yMax = 0, xMin = std::numeric_limits<
			int>::max(), yMin = std::numeric_limits<int>::max();
	int objectSize = (int) coordinatesX->Length();
	if (objectSize == 0)
		return;
	if (objectSize == 1) {
		boundingBoxMinX = coordinatesXValues[0];
		boundingBoxMinY = coordinatesYValues[0];
		boundingBoxMaxX = coordinatesXValues[0];
		boundingBoxMaxY = coordinatesYValues[0];
		iniX = coordinatesXValues[0];
		iniY = coordinatesYValues[0];
		//chainCode already set to empty vector
		chainCodeStr << "";
		return;
	}
	const char connectorCode[][2] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, {
			-1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } };
	for (int i = 0; i < objectSize; i++) {
		if (coordinatesXValues[i] < xMin)
			xMin = coordinatesXValues[i];
		if (coordinatesXValues[i] > xMax)
			xMax = coordinatesXValues[i];
		if (coordinatesYValues[i] < yMin)
			yMin = coordinatesYValues[i];
		if (coordinatesYValues[i] > yMax)
			yMax = coordinatesYValues[i];
	}

	boundingBoxMinX = xMin;
	boundingBoxMinY = yMin;
	boundingBoxMaxX = xMax;
	boundingBoxMaxY = yMax;
	width = xMax - xMin + 1;
	height = yMax - yMin + 1;

	std::vector<std::vector<char> > bitmap = std::vector<std::vector<char> >(
			height, std::vector<char>(width, 0));

	//Draw the objects into a table using the coordinate list
	for (int i = 0; i < objectSize; i++)
		bitmap[coordinatesYValues[i] - yMin][coordinatesXValues[i] - xMin] = 1;

	int xIni = 0, yIni = 0; //yIni = height - 1;
	//Find the first point to initialize contour tracing
	for (xIni = 0; xIni < width; xIni++)
		for (yIni = 0; yIni < height; yIni++)
			if (bitmap[yIni][xIni] != 0)
				break;

	iniX = (xIni + xMin);
	iniY = (yIni + yMin);
	int x = xIni, xOld = xIni, y = yIni, yOld = yIni;
	int dirOld = 4, dirNew = 5;
	std::vector<int> polygonXList;
	std::vector<int> polygonYList;
	polygonXList.push_back(x);
	polygonYList.push_back(y);

	int connectorLoopNum = 0;

	while (true) {

		x = xOld + connectorCode[dirNew][0];
		y = yOld + connectorCode[dirNew][1];
		//make sure that the coordinates are inside the pixel map.
		if (x >= 0 && x < width && y >= 0 && y < height) {
			if (bitmap[y][x] == 3) // cannot visit a point more than twice
				break;
			if (bitmap[y][x] > 0) {
				xOld = x;
				yOld = y;
				bitmap[y][x]++;
				chainCode->push_back(dirNew);
				chainCodeStr << dirNew;

				//Only store the coordinates on a direction change
				if (dirNew != dirOld) {
					polygonXList.push_back(x);
					polygonYList.push_back(y);
				} else {
					polygonXList.pop_back();
					polygonYList.pop_back();
					polygonXList.push_back(x);
					polygonYList.push_back(y);
				}
				if (x == xIni && y == yIni) {
					polygonXList.pop_back();
					polygonYList.pop_back();
					break;
				}

				dirOld = dirNew;
				// Reset Connector Loop
				connectorLoopNum = 0;
				// Set new direction as backward direction (will be incremented at the end of the loop)
				dirNew = (dirNew + 4) % 8;
			}
		}

		// Search pixels CCW from previous direction
		dirNew = (dirNew + 1) % 8;

		//handle cases of single points Maximum number of value is 8 (0..7).
		if (connectorLoopNum > 7)
			break;

		//increase the number of loop that was done over the connectorCode. Maximum number of value is 8 (0..7).
		connectorLoopNum++;
	}

	polygonXList.clear();
	polygonYList.clear();

}

// Module export
extern "C"
#ifdef _MSC_VER
__declspec(dllexport)
#endif
int ExportedModules(ModReg** x, int *Count) {
	static ModReg p[] = {
	// Declare module names and creation function addresses
			{ "ExtractROI", ExtractROI::Create }, };
	*x = p;
	*Count = sizeof(p) / sizeof(ModReg);
	return MOD_CLASS_LIBRARY;
}
