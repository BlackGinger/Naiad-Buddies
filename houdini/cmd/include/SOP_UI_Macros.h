
#ifndef __UI_MACROS_H__
#define __UI_MACROS_H__

//UI MACROS

#define GET_PARM_I(PrmName, FuncName) \
	int get##FuncName(float t) const\
	{ return evalInt((PrmName), 0, t); }
	
#define GET_PARM_F(PrmName, FuncName) \
	float get##FuncName(float t) const\
	{ return evalFloat((PrmName), 0, t); }

#define GET_PARM_V3(PrmName, FuncName) \
	UT_Vector3 get##FuncName(float t) const\
	{ \
		UT_Vector3 result; \
		for (int i = 0; i < 3; i ++) \
			result[i] = evalFloat((PrmName), i, t); \
		return result; \
	}

#define GET_PARM_S(PrmName, FuncName) \
	std::string get##FuncName(float t) const\
	{ \
		UT_String result; \
		evalString(result, (PrmName), 0, t); \
		return result.toStdString(); \
	} \

#endif

