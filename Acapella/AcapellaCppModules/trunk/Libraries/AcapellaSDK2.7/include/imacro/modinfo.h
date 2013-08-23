#ifndef _IMACRO_MODINFO_H_INCLUDED_
#define _IMACRO_MODINFO_H_INCLUDED_

// Self-reflection info functions exported from imacro.dll

namespace NIMacro {

/**  Compose a list of modules belonging to the specified informal group.
* Note that a module may belong to multiple groups.
* See also the Modules() module 
* for more flexible support.
*
* @param buffer Output string buffer.
* @param pszGroupName Specifies the group. If not found, an empty string is returned.
* @param cSep Names in the returned list are separated with cSep character 
*			unless it is zero.
*/
DI_IMacro int ListModulesOfGroup(Nbaseutil::safestring& buffer, const char* pszGroupName, char cSep);

/**  Compose a list of all informal module group names currently registered.
* See also Groups() module for more flexible support.
*
* @param buffer Output string buffer.
* @param cSep Names in the returned list are separated with cSep character 
*			unless it is zero.
*/
DI_IMacro int ListGroups(Nbaseutil::safestring& buffer, char cSep);

} // namespace

#endif
