
#ifndef __H__UG_BRIDGE__REGISTRY__
#define __H__UG_BRIDGE__REGISTRY__

#include <vector>
#include <cstring>

#include "global_function.h"
#include "class.h"
#include "param_to_type_value_list.h"
#include "parameter_stack.h"

namespace ug
{
namespace bridge
{

// Registry
/** registers functions and classes that are exported to scripts and visualizations
 *
 */
class Registry {
	public:
		Registry()	{}
	//////////////////////
	// global functions
	//////////////////////

	/**	References the template function proxy_function<TFunc> and stores
	 * it with the FuntionWrapper.
	 */
		template<class TFunc>
		Registry& add_function(const char* funcName, TFunc func, const char* group = "",
								const char* retValName = "", const char* paramValNames = "",
								const char* tooltip = "", const char* help = "",
								const char* retValInfoType = "", const char* paramValInfoType = "")
		{
		//	check that funcName is not already used
			if(functionname_registered(funcName))
			{
				std::cout << "### Registry ERROR: Trying to register function name '" << funcName
						<< "', that is already used by another function in this registry."
						<< "\n### Please change register process. Aborting ..." << std::endl;
				throw(UG_REGISTRY_ERROR_RegistrationFailed(funcName));
			}
		// 	check that name is not empty
			if(strlen(funcName) == 0)
			{
				std::cout << "### Registry ERROR: Trying to register empty function name."
						<< "\n### Please change register process. Aborting ..." << std::endl;
				throw(UG_REGISTRY_ERROR_RegistrationFailed(funcName));
			}

		//  create new exported function
			m_vFunction.push_back(new ExportedFunction(	func, &FunctionProxy<TFunc>::apply,
														funcName, group,
														retValName, paramValNames,
														tooltip, help,
														retValInfoType, paramValInfoType));
	
			return *this;
		}

	/// number of function registered at the Registry
		size_t num_functions() const						{return m_vFunction.size();}

	/// returns an exported function
		ExportedFunction& get_function(size_t ind) 			{return *m_vFunction.at(ind);}

	///////////////////
	// classes
	///////////////////

	/** Register a class at this registry
	 * This function registers any class
	 */
		template <typename TClass>
		ExportedClass_<TClass>& add_class_(const char* className, const char* group = "")
		{
		//	check that className is not already used
			if(classname_registered(className))
			{
				std::cout << "### Registry ERROR: Trying to register class name '" << className
						<< "', that is already used by another class in this registry."
						<< "\n### Please change register process. Aborting ..." << std::endl;
				throw(UG_REGISTRY_ERROR_RegistrationFailed(className));
			}
		// 	check that name is not empty
			if(strlen(className) == 0)
			{
				std::cout << "### Registry ERROR: Trying to register empty class name."
						<< "\n### Please change register process. Aborting ..." << std::endl;
				throw(UG_REGISTRY_ERROR_RegistrationFailed(className));
			}

		//	new class pointer
			ExportedClass_<TClass>* newClass = NULL;

		//	try creation
			try
			{
				newClass = new ExportedClass_<TClass>(className, group);
			}
			catch(ug::bridge::UG_REGISTRY_ERROR_ClassAlreadyNamed ex)
			{
				std::cout << "### Registry ERROR: Trying to register class with name '" << className
						<< "', that has already been named. This is not allowed. "
						<< "\n### Please change register process. Aborting ..." << std::endl;
				throw(UG_REGISTRY_ERROR_RegistrationFailed(className));
			}

		//	add new class to list of classes
			m_vClass.push_back(newClass);

			return *newClass;
		}

	/** Register a class at this registry
	 * This function registers any class together with its base class
	 */
		template <typename TClass, typename TBaseClass>
		ExportedClass_<TClass>& add_class_(const char* className, const char* group = "")
		{
		//	check that className is not already used
			if(classname_registered(className))
			{
				std::cout << "### Registry ERROR: Trying to register class name '" << className
						<< "', that is already used by another class in this registry."
						<< "\n### Please change register process. Aborting ..." << std::endl;
				throw(UG_REGISTRY_ERROR_RegistrationFailed(className));
			}
		// 	check that name is not empty
			if(strlen(className) == 0)
			{
				std::cout << "### Registry ERROR: Trying to register empty class name."
						<< "\n### Please change register process. Aborting ..." << std::endl;
				throw(UG_REGISTRY_ERROR_RegistrationFailed(className));
			}

		//	new class pointer
			ExportedClass_<TClass>* newClass = NULL;

		//	try creation of new class
			try
			{
				newClass = new ExportedClass_<TClass>(className, group);
			}
			catch(ug::bridge::UG_REGISTRY_ERROR_ClassAlreadyNamed ex)
			{
				std::cout << "### Registry ERROR: Trying to register class with name '" << className
						<< "', that has already been named. This is not allowed. "
						<< "\n### Please change register process. Aborting ..." << std::endl;
				throw(UG_REGISTRY_ERROR_RegistrationFailed(className));
			}

		// 	set base class names
			try
			{
				ClassNameProvider<TClass>::template set_name<TBaseClass>(className, group);
			}
			catch(ug::bridge::UG_REGISTRY_ERROR_ClassUnknownToRegistry ex)
			{
				std::cout <<"### Registry ERROR: Trying to register class with name '" << className
						<< "', that derives from class, that has not yet been registered to this Registry."
						<< "\n### Please change register process. Aborting ..." << std::endl;
				throw(UG_REGISTRY_ERROR_RegistrationFailed(className));
			}

		//	add new class to list of classes
			m_vClass.push_back(newClass);
			return *newClass;
		}

	/// number of classes registered at the Registry
		size_t num_classes() const						{return m_vClass.size();}

	/// returns an exported function
		const IExportedClass& get_class(size_t ind)	const {return *m_vClass.at(ind);}


	/// destructor
		~Registry()
		{
		//  delete registered functions
			for(size_t i = 0; i < m_vFunction.size(); ++i)
			{
				if(m_vFunction[i] != NULL)
					delete m_vFunction[i];
			}
		//  delete registered classes
			for(size_t i = 0; i < m_vClass.size(); ++i)
			{
				if(m_vClass[i] != NULL)
					delete m_vClass[i];
			}
		}

	protected:
		// returns true if classname is already used by a class in this registry
		bool classname_registered(const char* name)
		{
			for(size_t i = 0; i < m_vClass.size(); ++i)
			{
			//  compare strings
				if(strcmp(name, m_vClass[i]->name()) == 0)
					return true;
			}
			return false;
		}

		// returns true if functionname is already used by a function in this registry
		bool functionname_registered(const char* name)
		{
			for(size_t i = 0; i < m_vFunction.size(); ++i)
			{
			//  compare strings
				if(strcmp(name, (m_vFunction[i]->name()).c_str()) == 0)
					return true;
			}
			return false;
		}

	private:
		Registry(const Registry& reg)	{}
		
		std::vector<ExportedFunction*>	m_vFunction;

		std::vector<IExportedClass*> m_vClass;
};

} // end namespace registry

} // end namespace ug


#endif /* __H__UG_BRIDGE__REGISTRY__ */
