//
// DI_memblock_.h : main-header file for the memblock DLL                      
//							 not including exported headers         							
//																								
#if !defined(DI_memblock__INCLUDED_)												    
#define DI_memblock__INCLUDED_															
//																								
#pragma once																					
//																								
#define DI_memblock  __declspec(dllimport)										    	
#define BNS_memblock namespace Nmemblock {            							
#define ENS_memblock } using namespace Nmemblock;                   			
//																								
//																								
#ifdef _DEBUG																					
 #pragma comment(linker, "/defaultLib:memblockD.lib")	   								
#else      																					
 #pragma comment(linker, "/defaultLib:memblockR.lib")	   								
#endif     																					
//																								
//																								
//																								
//																								
//####### Include the headers you need in the .cpp-files off your choice #########	            
//####### down below you see a list off all the exported header-files    #########	            
//####### off 'memblock'                                         #########	            
//################################################################################     		
/*																								
// 
#include "DI_start.h"   
#include "container.h"   
#include "dataitem.h"   
#include "heap_dbg_start.h"   
#include "indexvector.h"   
#include "job.h"   
#include "jobpool.h"   
#include "memblock.h"   
#include "pmemblock.h"   
#include "table.h"   
#include "error.h"   
#include "intervalvector.h"   
#include "point.h"   
#include "safevalue.h"   
#include "triple.h"   
#include "fraction.h"   
#include "image.h"   
#include "numrange.h"   
#include "serializer.h"   
#include "vector.h"   
#include "memblock_export.h"   
#include "datacube.h"   
#include "datatype.h"   
#include "refcounted.h"   
#include "threadsharablepointer.h"   
#include "memblockimplbase.h"   
#include "memchunk.h"   
#include "typedchunk.h"   
#include "geomblock.h"   
#include "sharedlib.h"   
#include "statistic.h"   
#include "logger.h"   
#include "threadmgr.h"   
#include "timer.h"   
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
#endif // !defined(DI_memblock_INCLUDED_)											    
