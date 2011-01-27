//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m09 d16

#include <sstream>
#include <cstring>
#include <cstdlib>
#include "bindings_lua.h"
#include "ug_script/ug_script.h"
#include "ug_bridge/registry.h"
#include "common/common.h"
#include "../info_commands.h"

using namespace std;

namespace ug
{
namespace bridge
{
namespace lua
{

struct UserDataWrapper
{
	bool	is_const;
	void*	obj;
};


///	creates a new UserData_IObject and associates it with ptr in luas registry
/**
 * Creates a new userdata in lua, which encapsulates the given pointer.
 * It then assigns the specified metatable to the userdata.
 * When the function is done, the userdata is left on luas stack.
 */
static UserDataWrapper* CreateNewUserData(lua_State* L, void* ptr,
										  const char* metatableName,
										  bool is_const)
{
//	create the userdata
	UserDataWrapper* udata = (UserDataWrapper*)lua_newuserdata(L,
											sizeof(UserDataWrapper));
		
//	associate the object with the userdata.
	udata->obj = ptr;
	udata->is_const = is_const;
	
//	associate the metatable (userdata is already on the stack)
	luaL_getmetatable(L, metatableName);
	lua_setmetatable(L, -2);
	
	return udata;
}
	

/**
 *
 * \returns	String describing the content of the lua stack at a given index
 * and all types it is compatible with (for ex. "2" is string and number)
 */
string GetLuaTypeString(lua_State* L, int index)
{
	if(lua_isnil(L, index))
		return string("nil");
	string str("");
	// somehow lua_typeinfo always prints userdata
	if(lua_isfunction(L, index)) str.append("function/");
	if(lua_iscfunction(L, index)) str.append("cfunction/");
	if(lua_isboolean(L, index)) str.append("boolean/");
	if(lua_isnumber(L, index)) 	str.append("number/");
	if(lua_isstring(L, index)) str.append("string/");
	if(lua_islightuserdata(L, index)) str.append("lightuserdata/");
	if(lua_isuserdata(L, index))
	{
		if(lua_getmetatable(L, index) == 0)
			str.append("userdata/");
		else
		{
			// get names
			lua_pushstring(L, "names");
			lua_rawget(L, -2);

			if(lua_isnil(L, index) || !lua_isuserdata(L, index))
			{
				lua_pop(L, 2);
				str.append("userdata/");
			}
			else
			{
				const std::vector<const char*> *names = (const std::vector<const char*>*) lua_touserdata(L, -1);
				lua_pop(L, 2);
				if(names->size() <= 0) str.append("userdata/");
				else { str.append((*names)[0]); str.append("/"); }
			}
		}
	}
	if(lua_type(L, index) == LUA_TNONE)	str.append("none/");

	if(str.size() == 0)
		return string("unknown type");
	else
		return str.substr(0, str.size()-1);
}


///	copies parameter values from the lua-stack to a parameter-list.
/**	\returns	The index of the first bad parameter starting from 1.
 *				Returns 0 if everything went right.
 */
static int LuaStackToParams(ParameterStack& params,
							const ParameterStack& paramsTemplate,
							lua_State* L,
							 int offsetToFirstParam = 0)
{
	int badParam = 0;
	bool printDefaultParamErrorMsg = true;
	
//	iterate through the parameter list and copy the value in the associated
//	stack entry.
	for(int i = 0; i < paramsTemplate.size(); ++i){
		int type = paramsTemplate.get_type(i);
		int index = (int)i + offsetToFirstParam + 1;//stack-indices start with 1
		
		if(lua_type(L, index) == LUA_TNONE)
		{
			UG_LOG("ERROR: not enough parameters to function (got " << i << ", but needs " << paramsTemplate.size() << ")\n");
			return (int)i + 1;
		}

		switch(type){
			case PT_BOOL:{
				if(lua_isboolean(L, index))
					params.push_bool(lua_toboolean(L, index));
				else{
					UG_LOG("ERROR: type mismatch in argument " << i + 1 << ": expected bool, but given " << GetLuaTypeString(L, index) << "\n");
					badParam = (int)i + 1;
				}
			}break;
			case PT_INTEGER:{
				if(lua_isnumber(L, index))
					params.push_integer(lua_tonumber(L, index));
				else{
					UG_LOG("ERROR: type mismatch in argument " << i + 1 << ": expected number, but given " << GetLuaTypeString(L, index) << "\n");
					badParam = (int)i + 1;
				}
			}break;
			case PT_NUMBER:{
				if(lua_isnumber(L, index)){
					params.push_number(lua_tonumber(L, index));
				}
				else{
					UG_LOG("ERROR: type mismatch in argument " << i + 1 << ": expected number, but given " << GetLuaTypeString(L, index) << "\n");
					badParam = (int)i + 1;
				}
			}break;
			case PT_STRING:{
				if(lua_isstring(L, index))
					params.push_string(lua_tostring(L, index));
				else{
					badParam = (int)i + 1;
					UG_LOG("ERROR: type mismatch in argument " << i + 1 << ": expected string, but given " << GetLuaTypeString(L, index) << "\n");
				}
			}break;
			case PT_POINTER:{
				if(lua_isuserdata(L, index)){
				//	if the object is a const object, we can't use it here.
					if(((UserDataWrapper*)lua_touserdata(L, index))->is_const){
						UG_LOG("ERROR: Can't convert const object to non-const object.\n");
						badParam = (int)i + 1;
						break;
					}
					
				//	get the object and its metatable. Make sure that obj can be cast to
				//	the type that is expected by the paramsTemplate.
					void* obj = ((UserDataWrapper*)lua_touserdata(L, index))->obj;
					if(lua_getmetatable(L, index) != 0){

						lua_pushstring(L, "names");
						lua_rawget(L, -2);
						const std::vector<const char*>* names = (const std::vector<const char*>*) lua_touserdata(L, -1);
						lua_pop(L, 2);
						bool typeMatch = false;
						if(names){
							if(!names->empty()){
								if(ClassNameVecContains(*names, paramsTemplate.class_name(i)))
									typeMatch = true;
							}
						}

						if(typeMatch)
							params.push_pointer(obj, names);
						else{
							UG_LOG("ERROR: type mismatch in argument " << i + 1);
							UG_LOG(": Expected type that supports " << paramsTemplate.class_name(i));
							bool gotone = false;
							if(names){
								if(!names->empty()){
									gotone = true;
									UG_LOG(", but given object of type " << names->at(0) << ".\n");
								}
							}

							if(!gotone){
								UG_LOG(", but given object of unknown type.\n");
							}
							printDefaultParamErrorMsg = false;
							badParam = (int)i + 1;
						}
					}
					else{
						UG_LOG("ERROR: in argument " << i+1 << ": METATABLE not found. ");
						badParam = (int)i + 1;
					}
				}
				else
				{
					UG_LOG("ERROR: bad argument " << i+1 << ": Currently only references/pointers to user defined data allowed. ");
					UG_LOG("(Expected type that supports " << paramsTemplate.class_name(i) << ", but given " << GetLuaTypeString(L, index) << ")" << endl);
					badParam = (int)i + 1;
				}
			}break;
			case PT_CONST_POINTER:{
				if(lua_isuserdata(L, index)){
				//	get the object and its metatable. Make sure that obj can be cast to
				//	the type that is expected by the paramsTemplate.
					const void* obj = ((UserDataWrapper*)lua_touserdata(L, index))->obj;
					if(lua_getmetatable(L, index) != 0){

						lua_pushstring(L, "names");
						lua_rawget(L, -2);
						const std::vector<const char*>* names = (const std::vector<const char*>*) lua_touserdata(L, -1);
						lua_pop(L, 2);
						bool typeMatch = false;
						if(names){
							if(!names->empty()){
								if(ClassNameVecContains(*names, paramsTemplate.class_name(i)))
									typeMatch = true;
							}
						}

						if(typeMatch)
							params.push_const_pointer(obj, names);
						else{
							UG_LOG("ERROR: type mismatch in argument " << i + 1);
							UG_LOG(": Expected type that supports const " << paramsTemplate.class_name(i));
							bool gotone = false;
							if(names){
								if(!names->empty()){
									gotone = true;
									UG_LOG(", but given object of type " << names->at(0) << ".\n");
								}
							}

							if(!gotone){
								UG_LOG(", but given object of unknown type.\n");
							}
							printDefaultParamErrorMsg = false;
							badParam = (int)i + 1;
						}
					}
					else{
						UG_LOG("ERROR: in argument " << i+1 << ": METATABLE not found. ");
						badParam = (int)i + 1;
					}
				}
				else
				{
					UG_LOG("ERROR: bad argument " << i+1 << ": Currently only references/pointers to user defined data allowed. ");
					UG_LOG("(Expected type that supports " << paramsTemplate.class_name(i) << ")" << endl);
					badParam = (int)i + 1;
				}
			}break;
			default:{//	unknown type
				badParam = (int)i + 1;
			}break;
		}
		if(badParam)
			return badParam;
	}
		
	return badParam;
}

///	Pushes the parameter-values to the Lua-Stack.
/**
 * \returns The number of items pushed to the stack.
 */
static int ParamsToLuaStack(const ParameterStack& params, lua_State* L)
{
//	push output parameters to the stack	
	for(int i = 0; i < params.size(); ++i){
		int type = params.get_type(i);
		switch(type){
			case PT_BOOL:{
				lua_pushboolean(L, (params.to_bool(i)) ? 1 : 0);
			}break;
			case PT_INTEGER:{
				lua_pushnumber(L, params.to_integer(i));
			}break;
			case PT_NUMBER:{
				lua_pushnumber(L, params.to_number(i));
			}break;
			case PT_STRING:{
				lua_pushstring(L, params.to_string(i));
			}break;
			case PT_POINTER:{
				void* obj = params.to_pointer(i);
				CreateNewUserData(L, obj, params.class_name(i), false);
			}break;
			case PT_CONST_POINTER:{
			//	we're removing const with a cast. However, it was made sure that
			//	obj is treated as a const value.
				void* obj = (void*)params.to_const_pointer(i);
				CreateNewUserData(L, obj, params.class_name(i), true);
			}break;
			default:{
				UG_LOG("ERROR in ParamsToLuaStack: Unknown parameter in ParameterList. ");
				UG_LOG("Return-values may be incomplete.\n");
				return (int)i;
			}break;
		}
	}
	
	return (int)params.size();
}

static int LuaProxyFunction(lua_State* L)
{
	const ExportedFunction* func = (const ExportedFunction*)lua_touserdata(L, lua_upvalueindex(1));

	ParameterStack paramsIn;;
	ParameterStack paramsOut;
	
	int badParam = LuaStackToParams(paramsIn, func->params_in(), L, 0);
	
//	check whether the parameter was correct
	if(badParam > 0){
		UG_LOG("ERROR occured during call to ");
		PrintFunctionInfo(*func);
		UG_LOG(".\n");
		return 0;
	}

	try{
		func->execute(paramsIn, paramsOut);
	}
	catch(UGError err){
		UG_LOG("UGError in ")
		PrintFunctionInfo(*func);
		UG_LOG(" with code " << err.get_code() << ": ");
		UG_LOG(err.get_msg() << endl);
		if(err.terminate())
		{
			UG_LOG("terminating..." << endl);
			exit(err.get_code());
		}
	}
	catch(...)
	{
		UG_LOG("unknown error occured in call to ")
		PrintFunctionInfo(*func);
		UG_LOG(". continuing execution...\n");
	}

	return ParamsToLuaStack(paramsOut, L);
}

static int LuaProxyConstructor(lua_State* L)
{
	IExportedClass* c = (IExportedClass*)lua_touserdata(L, lua_upvalueindex(1));
	
	CreateNewUserData(L, c->create(), c->name(), false);

	return 1;
}

static int LuaProxyMethod(lua_State* L)
{
	const ExportedMethod* m = (const ExportedMethod*)lua_touserdata(L, lua_upvalueindex(1));

	if(!lua_isuserdata(L, 1))
	{
		UG_LOG("ERROR in call to LuaProxyMethod: No object specified in call to ");
		PrintLuaClassMethodInfo(L, 1, *m);
		UG_LOG(".\n");
		return 0;
	}
	
	UserDataWrapper* self = (UserDataWrapper*)lua_touserdata(L, 1);
	
	ParameterStack paramsIn;;
	ParameterStack paramsOut;
	
	int badParam = LuaStackToParams(paramsIn, m->params_in(), L, 1);
	
//	check whether the parameter was correct
	if(badParam > 0)
	{

		UG_LOG("ERROR occured during call to ");
		PrintLuaClassMethodInfo(L, 1, *m);
		UG_LOG(".\n");
		return 0;
	}

	try{
		m->execute(self->obj, paramsIn, paramsOut);
	}
	catch(UGError err)
	{
		UG_LOG("UGError in ")
		PrintLuaClassMethodInfo(L, 1, *m);
		UG_LOG(" with code " << err.get_code() << ": ");
		UG_LOG(err.get_msg() << endl);
		if(err.terminate())
		{
			UG_LOG("terminating..." << endl);
			exit(err.get_code());
		}
	}
	catch(...)
	{
		UG_LOG("unknown error occured in call to ");
		PrintLuaClassMethodInfo(L, 1, *m);
		UG_LOG(". continuing execution...\n");
	}


	return ParamsToLuaStack(paramsOut, L);
}

static int MetatableIndexer(lua_State*L)
{
//	the stack contains the object and the requested key.
//	we have to make sure to only call const methods on const objects
	bool is_const = ((UserDataWrapper*)lua_touserdata(L, 1))->is_const;
	
//	first we push the objects metatable onto the stack.
	lua_getmetatable(L, 1);

//	now we have to traverse the class hierarchy
	while(1)
	{
	//	if the object is not const, check whether the key is in the current metatable
		if(!is_const){
			lua_pushvalue(L, 2);
			lua_rawget(L, -2);
		
		//	if we retrieved something != nil, we're done.
			if(!lua_isnil(L, -1))
				return 1;
	
			lua_pop(L, 1);
		}
		
	//	the next thing to check are the const methods
		lua_pushstring(L, "__const");
		lua_rawget(L, -2);
		if(!lua_isnil(L, -1)){
		//	check whether the entry is contained in the table
			lua_pushvalue(L, 2);
			lua_rawget(L, -2);
		
		//	if we retrieved something != nil, we're done.
			if(!lua_isnil(L, -1)){
				return 1;
			}
				
		//	remove nil from stack
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
		
	//	the requested key has not been in the metatable.
	//	we thus have to check the parents metatable.
		lua_pushstring(L, "__parent");
		lua_rawget(L, -2);
		
	//	if we retrieved a parent-table, we will remove the old table
	//	and rerun the iteration. If not, we're done in here.
	//	if no method has been found, nil should be returned.
		if(lua_isnil(L, -1))
			return 1;
		
	//	remove the old metatable
		lua_remove(L, -2);
	}
}

bool CreateBindings_LUA(lua_State* L, Registry& reg)
{
//	registers a meta-object for each object found in the ObjectRegistry.
//	Global functions are registered for all GlobalFunction-objects in the registry.

//	iterate through all registered objects
	
/*
//	create the ug-table in which all our methods will go.
	lua_newtable(L);
//	set the name of the table
	lua_setglobal(L, "ug");
	*/
////////////////////////////////
//	create a method for each global function, which calls LuaProxyFunction with
//	a an index to the function.
	size_t numFuncs = reg.num_functions();
	
	for(size_t i = 0; i < numFuncs; ++i){
		ExportedFunction* func = &reg.get_function(i);

	//	check whether the function already exists
		lua_getglobal(L, func->name().c_str());
		if(!lua_isnil(L, -1)){
		//	the method already exists. Don't recreate it.
			lua_pop(L, 1);
			continue;
		}
		lua_pop(L, 1);
		
	//	the function is new. Register it.
		lua_pushlightuserdata(L, func);
		lua_pushcclosure(L, LuaProxyFunction, 1);
		lua_setglobal(L, func->name().c_str());
	}


	size_t numClasses = reg.num_classes();
	for(size_t i = 0; i < numClasses; ++i){
		const IExportedClass* c = &reg.get_class(i);
		
	//	check whether the class already exists
		lua_getglobal(L, c->name());
		if(!lua_isnil(L, -1)){
		//	the class already exists. Don't recreate it.
			lua_pop(L, 1);
			continue;
		}
		lua_pop(L, 1);
		
	//	The class is new. Register it.
		if(c->is_instantiable())
		{
		//	set the constructor-function
			lua_pushlightuserdata(L, (void*)c);
			lua_pushcclosure(L, LuaProxyConstructor, 1);
			lua_setglobal(L, c->name());
		}
		
	//	create the meta-table for the object
	//	overwrite index and store the class-name
		luaL_newmetatable(L, c->name());
	//	we use our custom indexing method to allow method-derivation
		lua_pushcfunction(L, MetatableIndexer);
		lua_setfield(L, -2, "__index");
	//	we have to store the class-names of the class hierarchy
		lua_pushstring(L, "names");
		lua_pushlightuserdata(L, (void*)c->class_names());
		lua_settable(L, -3);
	//	if the class has a parent, we'll store its metatable in the __parent entry.
		const vector<const char*>* classNames = c->class_names();
		if(classNames){
			if(classNames->size() > 1){
			//	push the entries name
				lua_pushstring(L, "__parent");
			//	get the metatable of the parent
				luaL_getmetatable(L, classNames->at(1));
			//	assign the entry
				lua_settable(L, -3);
			}
		}
		
	//	register methods
		for(size_t j = 0; j < c->num_methods(); ++j){
			const ExportedMethod& m = c->get_method(j);
			lua_pushstring(L, m.name().c_str());
			lua_pushlightuserdata(L, (void*)&m);
			lua_pushcclosure(L, LuaProxyMethod, 1);
			lua_settable(L, -3);
		}
		
	//	register const-methods
		if(c->num_const_methods() > 0)
		{
		//	create a new table in the meta-table and store it in the entry __const
			lua_newtable(L);
			for(size_t j = 0; j < c->num_const_methods(); ++j){
				const ExportedMethod& m = c->get_const_method(j);
				lua_pushstring(L, m.name().c_str());
				lua_pushlightuserdata(L, (void*)&m);
				lua_pushcclosure(L, LuaProxyMethod, 1);
				lua_settable(L, -3);
			}
			lua_setfield(L, -2, "__const");
		}
		
	//	pop the metatable from the stack.
		lua_pop(L, 1);
	}
	
	return true;
}

}//	end of namespace
}//	end of namespace
}//	end of namespace
