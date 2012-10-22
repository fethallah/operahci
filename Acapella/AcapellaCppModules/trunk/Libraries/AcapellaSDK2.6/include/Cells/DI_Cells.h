//
// DI_Cells.h : main-header file for the Cells DLL                       
//							including all exported headers										
//																								
#if !defined(DI_Cells_INCLUDED_)												    
#define DI_Cells_INCLUDED_															
//																								
#pragma once																					
//																								
#define DI_Cells  __declspec(dllimport)										    	
#define BNS_Cells namespace NCells {            							
#define ENS_Cells } using namespace NCells;                   			
//																								
//																								
#ifdef _DEBUG												
	#if defined (_UNICODE) 	|| defined (UNICODE)								
 		#pragma comment(linker, "/defaultLib:CellsDU.lib")	   				
#else													
 		#pragma comment(linker, "/defaultLib:CellsD.lib")	   				
	#endif     												
#else      												
	#if defined (_UNICODE) 	|| defined (UNICODE)								
		#pragma comment(linker, "/defaultLib:CellsRU.lib")	   				
#else													
 		#pragma comment(linker, "/defaultLib:CellsR.lib")	   				
	#endif     												
#endif     												
//																								
//																								
//																								
//																								
//####### Exported headers the dll-client must includes to use 'Cells' #########	
//#####################################################################################		
// 
#include "cells.h"   
#include "bitvector.h"   
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
