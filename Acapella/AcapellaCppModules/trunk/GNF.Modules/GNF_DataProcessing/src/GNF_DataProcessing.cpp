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
			std::stringstream& boundingBoxMin,
			std::stringstream& boundingBoxMax,
			std::stringstream& initCoordinate, std::stringstream& chain,
			std::stringstream& coordinates);
private:
	// module parameters
	const PIntervalVector stencil;
	PVector boundingBoxMinCoordinateList;
	PVector boundingBoxMaxCoordinateList;
	PVector inialCoordinate;
	PVector chainCode;
	PVector coordinatesList;
};

void ExtractROI::Declare() {
	// Declare module groups and general description
	module(
			0,
			"Object list manipulations, GNF_Dataprocessing",
			"Returns a vector with the coordinates for the stencil provided as a polygon. Coordinates are reported in the for \"X;Y\" and the coordinate system is: (X=0;Y=0) corresponds to the top left and Y increases going down, X increases going up. This follows Acapella nomenclature.");
	// Declare module input parameters
	input(stencil, "Stencil (intervalvector)", PAR_POSITIONAL,
			"Stencil containing the border of the ROI to extract.");
	// Declare module output parameters

	output(
			boundingBoxMinCoordinateList,
			"boundingBoxMinCoordinateList (vector: string)",
			0,
			"Vector containing the smallest coordinates for each ROI bounding box (xMin;yMin).");
	output(
			boundingBoxMaxCoordinateList,
			"boundingBoxMaxCoordinateList (vector: string)",
			0,
			"Vector containing the largest coordinates for each ROI bounding box (xMax;yMax).");
	output(inialCoordinate, "inialCoordinate (vector: string)", 0,
			"Vector containing the Initial coordinate use by the chain code");
	output(
			chainCode,
			"chainCode (vector: string)",
			0,
			"Vector containing the chaincode as text (Chain code is counter clockwise and starts at 0 move by x+1 and y+0.");
	output(coordinatesList, "coordinatesList (vector: string)", 0,
			"Vector containing the coordinates for each ROI returned as a chain code.");
}

void ExtractROI::Run() {

	const int* inner, *outer;
	unpack_two_layer_const(stencil, inner, outer);
	int objectNum = stencil->IntervalCount();
	boundingBoxMinCoordinateList = Vector::Create(objectNum, Vector::String);
	safestring* boundingBoxMinCoordinateListValue =
			boundingBoxMinCoordinateList->StringPointer();
	boundingBoxMaxCoordinateList = Vector::Create(objectNum, Vector::String);
	safestring* boundingBoxMaxCoordinateListValue =
			boundingBoxMaxCoordinateList->StringPointer();
	inialCoordinate = Vector::Create(objectNum, Vector::String);
	safestring* inialCoordinateValue = inialCoordinate->StringPointer();
	chainCode = Vector::Create(objectNum, Vector::String);
	safestring* chainCodeValue = chainCode->StringPointer();
	coordinatesList = Vector::Create(objectNum, Vector::String);
	safestring* coordinatesListValue = coordinatesList->StringPointer();
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
		std::stringstream boundingBoxMin, boundingBoxMax, chain,
				initCoordinate, coordinates;
		ExtractROI::polygonize(coordinatesX, coordinatesY, boundingBoxMin,
				boundingBoxMax, initCoordinate, chain, coordinates);
		boundingBoxMinCoordinateListValue[i - 1] = Printf("%d")(
				boundingBoxMin.str());
		boundingBoxMaxCoordinateListValue[i - 1] = Printf("%d")(
				boundingBoxMax.str());
		chainCodeValue[i - 1] = Printf("%d")(chain.str());
		inialCoordinateValue[i - 1] = Printf("%d")(initCoordinate.str());
		coordinatesListValue[i - 1] = Printf("%d")(coordinates.str());

	}
}

void ExtractROI::polygonize(PVector coordinatesX, PVector coordinatesY,
		std::stringstream& boundingBoxMin, std::stringstream& boundingBoxMax,
		std::stringstream& initCoordinate, std::stringstream& chain,
		std::stringstream& coordinates) {

	int* coordinatesXValues = coordinatesX->IntPointer();
	int* coordinatesYValues = coordinatesY->IntPointer();

	int width = 0, height = 0, xMax = 0, yMax = 0, xMin = std::numeric_limits<
			int>::max(), yMin = std::numeric_limits<int>::max();
	int objectSize = (int) coordinatesX->Length();
	if (objectSize == 0)
		return;
	if (objectSize == 1) {
		coordinates << coordinatesXValues[0] << ";" << coordinatesYValues[0];
		boundingBoxMin << coordinatesXValues[0] << ";" << coordinatesYValues[0];
		boundingBoxMax << coordinatesXValues[0] << ";" << coordinatesYValues[0];
		initCoordinate << coordinatesXValues[0] << ";" << coordinatesYValues[0];
		chain << "";
		return;
	}
	const char connectorCode[][2] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 },
			{ -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } };
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

	boundingBoxMin << xMin << ";" << yMin;
	boundingBoxMax << xMax << ";" << yMax;
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

	initCoordinate << (xIni + xMin) << ";" << (yIni + yMin);
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
				chain << dirNew;

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
				connectorLoopNum=0;
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

	for (int i = 0; i < (int) polygonXList.size(); i++)
		coordinates << (polygonXList[i] + xMin) << ";" << (polygonYList[i] + yMin) << ",";


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
