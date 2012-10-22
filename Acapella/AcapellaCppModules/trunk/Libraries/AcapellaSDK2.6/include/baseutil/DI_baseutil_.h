//
// DI_baseutil_.h : main-header file for the baseutil DLL                      
//							 not including exported headers         							
//																								
#if !defined(DI_baseutil__INCLUDED_)												    
#define DI_baseutil__INCLUDED_															
//																								
#pragma once																					
//																								
#define DI_baseutil  __declspec(dllimport)										    	
#define BNS_baseutil namespace Nbaseutil {            							
#define ENS_baseutil } using namespace Nbaseutil;                   			
//																								
//																								
#ifdef _DEBUG																					
 #pragma comment(linker, "/defaultLib:baseutilD.lib")	   								
#else      																					
 #pragma comment(linker, "/defaultLib:baseutilR.lib")	   								
#endif     																					
//																								
//																								
//																								
//																								
//####### Include the headers you need in the .cpp-files off your choice #########	            
//####### down below you see a list off all the exported header-files    #########	            
//####### off 'baseutil'                                         #########	            
//################################################################################     		
/*																								
// 
#include "heap_dbg_start.h"   
#include "config.h"   
#include "encodings.h"   
#include "safestring.h"   
#include "imacroexception.h"   
#include "mb_malloc.h"   
#include "printfer.h"   
#include "inputstream.h"   
#include "outputstream.h"   
#include "gcc.h"   
#include "utftranslator.h"   
#include "asocket.h"   
#include "carrier.h"   
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
#endif // !defined(DI_baseutil_INCLUDED_)											    
