//
// DI_IMacro_.h : main-header file for the IMacro DLL                      
//							 not including exported headers         							
//																								
#if !defined(DI_IMacro__INCLUDED_)												    
#define DI_IMacro__INCLUDED_															
//																								
#pragma once																					
//																								
#define DI_IMacro  __declspec(dllimport)										    	
#define BNS_IMacro namespace NIMacro {            							
#define ENS_IMacro } using namespace NIMacro;                   			
//																								
//																								
#ifdef _DEBUG																					
 #pragma comment(linker, "/defaultLib:IMacroD.lib")	   								
#else      																					
 #pragma comment(linker, "/defaultLib:IMacroR.lib")	   								
#endif     																					
//																								
//																								
//																								
//																								
//####### Include the headers you need in the .cpp-files off your choice #########	            
//####### down below you see a list off all the exported header-files    #########	            
//####### off 'IMacro'                                         #########	            
//################################################################################     		
/*																								
// 
#include "DI_start.h"   
#include "modreg.h"   
#include "imacro.h"   
#include "jar.h"   
#include "modinfo.h"   
#include "script.h"   
#include "datablock.h"   
#include "executive.h"   
#include "instance.h"   
#include "mod.h"   
#include "expressionparser.h"   
#include "regexp.h"   
#include "tmpl_helpers.h"   
#include "modpar.h"   
#include "dataset.h"   
#include "dbconn.h"   
#include "licfield.h"   
#include "errlogger.h"   
#include "unittest.h"   
#include "mrulist.h"   
#include "htmltemplatesmgr.h"   
#include "html_output.h"   
#include "debugger.h"   
#include "modulecall.h"   
#include "scriptfile.h"   
#include "resolvepoint.h"   
#include "textholder.h"   
#include "explicitargument.h"   
#include "package.h"   
#include "regexp.h"   
#include "closure.h"   
#include "shared_counter.h"   
#include "DI_finish.h"   
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
#endif // !defined(DI_IMacro_INCLUDED_)											    
