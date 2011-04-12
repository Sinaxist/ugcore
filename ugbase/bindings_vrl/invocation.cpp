#include "invocation.h"
#include "ug_bridge/class.h"
#include "type_converter.h"
#include <string>

namespace ug {
namespace vrl {
namespace invocation {

//static std::map<const char*, const ug::bridge::ExportedMethod*> methods;
//static std::map<const char*, const ug::bridge::ExportedFunction*> functions;
//
//const std::string createMethodSignature(JNIEnv* env, const char* className,
//		const char* methodName, bool readOnly, jobjectArray params) {
//	std::stringstream signature;
//
//	signature << className << "::" << methodName << "::const=" << readOnly;
//	size_t size = env->GetArrayLength(params);
//
//	for (size_t i = 0; i < size; i++) {
//
//		jobject param = env->GetObjectArrayElement(params, i);
//
//		signature << "::" << paramClass2ParamType(env, param);
//	}
//
//	return signature.str();
//}

const ug::bridge::ExportedMethod* getMethodBySignature(
		JNIEnv *env,
		ug::bridge::Registry* reg,
		const ug::bridge::IExportedClass* clazz, bool readOnly,
		std::string methodName,
		jobjectArray params) {

	//	// create signature
	//	std::string signature = createMethodSignature(
	//			env, clazz->name(), methodName.c_str(), readOnly, params);
	//
	//	// search in map first and return result if entry exists
	//	if (methods.find(signature.c_str()) != methods.end()) {
	//		UG_LOG("FOUND:" << signature << std::endl);
	//		return methods[signature.c_str()];
	//	}

	// we allow invocation of methods defined in parent classes
	std::vector<const ug::bridge::IExportedClass*> classes =
			getParentClasses(reg, clazz);

	// iterate over all classes of the inheritance path
	for (unsigned i = 0; i < classes.size(); i++) {

		const ug::bridge::IExportedClass* cls = classes[i];
		unsigned int numMethods = 0;

		// check whether to search const or non-const methods
		if (readOnly) {
			numMethods = cls->num_const_methods();
		} else {
			numMethods = cls->num_methods();
		}

		for (unsigned int j = 0; j < numMethods; j++) {
			const ug::bridge::ExportedMethod* method = NULL;

			unsigned int numOverloads = 1;

			if (readOnly) {
				numOverloads = cls->num_const_overloads(j);
			} else {
				numOverloads = cls->num_overloads(j);
			}

			for (unsigned int k = 0; k < numOverloads; k++) {

				// check whether to search const or non-const methods
				if (readOnly) {
					method = &cls->get_const_overload(j, k);
				} else {
					method = &cls->get_overload(j, k);
				}

				// if the method name and the parameter types are equal
				// we found the correct method
				if (strcmp(method->name().c_str(),
						methodName.c_str()) == 0 &&
						compareParamTypes(
						env, params, method->params_in())) {

					//	// improve lookup time: add method signature to map
					//	methods[signature.c_str()] = method;

					return method;
				}
			} // for k
		} // for j
	} // for i

	return NULL;
}

const ug::bridge::ExportedFunction* getFunctionBySignature(
		JNIEnv *env,
		ug::bridge::Registry* reg,
		std::string functionName,
		jobjectArray params) {


	//	// create signature
	//	std::string signature = createMethodSignature(
	//			env, "", "", false, params);
	//
	//	// search in map first and return result if entry exists
	//	if (methods.find(signature.c_str()) != methods.end()) {
	//		UG_LOG("FOUND:" << signature << std::endl);
	//		return functions[signature.c_str()];
	//	}

	unsigned int numFunctions = 0;

	numFunctions = reg->num_functions();

	for (unsigned int i = 0; i < numFunctions; i++) {
		const ug::bridge::ExportedFunction* func = NULL;

		unsigned int numOverloads = 1;

		numOverloads = reg->num_overloads(i);

		for (unsigned int k = 0; k < numOverloads; k++) {
			func = &reg->get_overload(i, k);

			// if the function name and the parameter types are equal
			// we found the correct function
			if (strcmp(func->name().c_str(),
					functionName.c_str()) == 0 &&
					compareParamTypes(
					env, params, func->params_in())) {

				// improve lookup time
				//			functions[signature.c_str()] = func;
				return func;
			}
		}
	}

	return NULL;
}

const ug::bridge::IExportedClass* getExportedClassPtrByName(
		ug::bridge::Registry* reg,
		std::string className) {

	for (unsigned int i = 0; i < reg->num_classes(); i++) {

		const ug::bridge::IExportedClass& clazz = reg->get_class(i);

		if (strcmp(clazz.name(), className.c_str()) == 0) {
			return &clazz;
		}
	}

	return NULL;
}

const ug::bridge::ClassNameNode* getClassNodePtrByName(
		ug::bridge::Registry* reg,
		std::string className) {

	for (unsigned int i = 0; i < reg->num_classes(); i++) {

		const ug::bridge::IExportedClass& clazz = reg->get_class(i);

		if (strcmp(clazz.name(), className.c_str()) == 0) {
			return &clazz.class_name_node();
		}
	}

	return NULL;
}

} // invocation::
} // vrl::
} // ug::
