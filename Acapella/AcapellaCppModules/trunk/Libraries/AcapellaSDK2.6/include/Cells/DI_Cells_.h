//
// DI_Cells_.h : main-header file for the Cells DLL                      
//							 not including exported headers         							
//																								
#if !defined(DI_Cells__INCLUDED_)												    
#define DI_Cells__INCLUDED_															
//																								
#pragma once																					
//																								
#define DI_Cells  __declspec(dllimport)										    	
#define BNS_Cells namespace NCells {            							
#define ENS_Cells } using namespace NCells;                   			
//																								
//																								
#ifdef _DEBUG																					
 #pragma comment(linker, "/defaultLib:CellsD.lib")	   								
#else      																					
 #pragma comment(linker, "/defaultLib:CellsR.lib")	   								
#endif     																					
//																								
//																								
//																								
//																								
//####### Include the headers you need in the .cpp-files off your choice #########	            
//####### down below you see a list off all the exported header-files    #########	            
//####### off 'Cells'                                         #########	            
//################################################################################     		
/*																								
// 
#include "cells.h"   
#include "bitvector.h"   
*/																								
//																								
//																								
//##  Don't Edit this file.  !!!!                                                          	
//##  It is generated bei the ExportD Command started bei CustomBuild-settings at 	            
//##  your last DllBuild.  If you want to change the headers to export,  edit the         	    
//##  ExportHeader-Parameterlist for 'ExportD' in  project-
//##  in your dll-project		    															
//																								
//																								
//																								
#endif // !defined(DI_Cells_INCLUDED_)											    
