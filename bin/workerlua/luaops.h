#ifndef _FF_LUA_OPS_H_
#define _FF_LUA_OPS_H_

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <lua.hpp>
namespace ff{

#define ADDR_ARG_POS(x) (x)
typedef int (*lua_func_t) (lua_State *L);

template<typename T>
struct luacpp_op_t;

template<>
struct luacpp_op_t<const char*>
{
    static void cpp2luastack(lua_State* ls_, const char* arg_)
    {
        lua_pushstring(ls_, arg_);
    }
    static int lua2cpp(lua_State* ls_, int pos_, char*& param_)
    {
        const char* str = luaL_checkstring(ls_, pos_);
        param_ = (char*)str;
        return 0;
    }
};

template<> struct luacpp_op_t<int8_t>
{

	static void cpp2luastack(lua_State* ls_, int8_t arg_)
	{
		lua_pushnumber(ls_, (lua_Number)arg_);
	}
	static int luareturn2cpp(lua_State* ls_, int pos_, int8_t& param_)
	{
		if (!lua_isnumber(ls_, pos_))
		{
			return -1;
		}
		param_ = (int8_t)lua_tonumber(ls_, pos_);
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, int8_t& param_)
	{
		param_ = (int8_t)luaL_checknumber(ls_, pos_);
		return 0;
	}
};

template<>
struct luacpp_op_t<uint8_t>
{
	static void cpp2luastack(lua_State* ls_, uint8_t arg_)
	{
		lua_pushnumber(ls_, (lua_Number)arg_);
	}
	static int luareturn2cpp(lua_State* ls_, int pos_, uint8_t& param_)
	{
		if (!lua_isnumber(ls_, pos_))
		{
			return -1;
		}
		param_ = (uint8_t)lua_tonumber(ls_, pos_);
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, uint8_t& param_)
	{
		param_ = (uint8_t)luaL_checknumber(ls_, pos_);
		return 0;
	}
};
template<> struct luacpp_op_t<int16_t>
{
	static void cpp2luastack(lua_State* ls_, int16_t arg_)
	{
		lua_pushnumber(ls_, (lua_Number)arg_);
	}
	static int luareturn2cpp(lua_State* ls_, int pos_, int16_t& param_)
	{
		if (!lua_isnumber(ls_, pos_))
		{
			return -1;
		}
		param_ = (int16_t)lua_tonumber(ls_, pos_);
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, int16_t& param_)
	{
		param_ = (int16_t)luaL_checknumber(ls_, pos_);
		return 0;
	}
};
template<> struct luacpp_op_t<uint16_t>
{

	static void cpp2luastack(lua_State* ls_, uint16_t arg_)
	{
		lua_pushnumber(ls_, (lua_Number)arg_);
	}
	static int luareturn2cpp(lua_State* ls_, int pos_, uint16_t& param_)
	{
		if (!lua_isnumber(ls_, pos_))
		{
			return -1;
		}
		param_ = (uint16_t)lua_tonumber(ls_, pos_);
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, uint16_t& param_)
	{
		param_ = (uint16_t)luaL_checknumber(ls_, pos_);
		return 0;
	}
};
template<> struct luacpp_op_t<int32_t>
{
	static void cpp2luastack(lua_State* ls_, int32_t arg_)
	{
		lua_pushnumber(ls_, (lua_Number)arg_);
	}
	static int luareturn2cpp(lua_State* ls_, int pos_, int32_t& param_)
	{
		if (!lua_isnumber(ls_, pos_))
		{
			return -1;
		}
		param_ = (int32_t)lua_tonumber(ls_, pos_);
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, int32_t& param_)
	{
		param_ = (int32_t)luaL_checknumber(ls_, pos_);
		return 0;
	}
};
template<> struct luacpp_op_t<uint32_t>
{

	static void cpp2luastack(lua_State* ls_, uint32_t arg_)
	{
		lua_pushnumber(ls_, (lua_Number)arg_);
	}
	static int luareturn2cpp(lua_State* ls_, int pos_, uint32_t& param_)
	{
		if (!lua_isnumber(ls_, pos_))
		{
			return -1;
		}
		param_ = (uint32_t)lua_tonumber(ls_, pos_);
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, uint32_t& param_)
	{
		param_ = (uint32_t)luaL_checknumber(ls_, pos_);
		return 0;
	}
};

template<>
struct luacpp_op_t<bool>
{
    static void cpp2luastack(lua_State* ls_, bool arg_)
    {
        lua_pushboolean(ls_, arg_);
    }

    static int luareturn2cpp(lua_State* ls_, int pos_, bool& param_)
    {
    	if (lua_isnil(ls_, pos_))
    	{
    		param_ = false;
    		return 0;
    	}
        if (!lua_isboolean(ls_, pos_))
        {
            return -1;
        }

        param_ = (bool)lua_toboolean(ls_, pos_);
        return 0;
    }
    static int lua2cpp(lua_State* ls_, int pos_, bool& param_)
    {
		luaL_checktype(ls_, pos_,  LUA_TBOOLEAN);
        param_ = (bool)lua_toboolean(ls_, pos_);
        return 0;
    }
};

template<>
struct luacpp_op_t<std::string>
{

    static void cpp2luastack(lua_State* ls_, const std::string& arg_)
    {
        lua_pushlstring(ls_, arg_.c_str(), arg_.length());
    }

    static int luareturn2cpp(lua_State* ls_, int pos_, std::string& param_)
    {
        if (!lua_isstring(ls_, pos_))
        {
            return -1;
        }

        lua_pushvalue(ls_, pos_);
        size_t len  = 0;
        const char* src = lua_tolstring(ls_, -1, &len);
        param_.assign(src, len);
        lua_pop(ls_, 1);

        return 0;
    }
    static int lua2cpp(lua_State* ls_, int pos_, std::string& param_)
    {
        size_t len = 0;
        const char* str = luaL_checklstring(ls_, pos_, &len);
        param_.assign(str, len);
        return 0;
    }
};
template<> struct luacpp_op_t<int64_t>
{
	static void cpp2luastack(lua_State* ls_, int64_t arg_)
	{
		lua_pushnumber(ls_, (lua_Number)arg_);
	}
	static int luareturn2cpp(lua_State* ls_, int pos_, int64_t& param_)
	{
		if (!lua_isnumber(ls_, pos_))
		{
			return -1;
		}
		param_ = (int64_t)lua_tonumber(ls_, pos_);
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, int64_t& param_)
	{
		param_ = (int64_t)luaL_checknumber(ls_, pos_);
		return 0;
	}
};
template<> struct luacpp_op_t<uint64_t>
{

	static void cpp2luastack(lua_State* ls_, uint64_t arg_)
	{
		lua_pushnumber(ls_, (lua_Number)arg_);
	}
	static int luareturn2cpp(lua_State* ls_, int pos_, uint64_t& param_)
	{
		if (!lua_isnumber(ls_, pos_))
		{
			return -1;
		}
		param_ = (uint64_t)lua_tonumber(ls_, pos_);
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, uint64_t& param_)
	{
		param_ = (uint64_t)luaL_checknumber(ls_, pos_);
		return 0;
	}
};

template<>
struct luacpp_op_t<const std::string&>
{
    static void cpp2luastack(lua_State* ls_, const std::string& arg_)
    {
        lua_pushlstring(ls_, arg_.c_str(), arg_.length());
    }

    static int luareturn2cpp(lua_State* ls_, int pos_, std::string& param_)
    {
        if (!lua_isstring(ls_, pos_))
        {
            return -1;
        }

        lua_pushvalue(ls_, pos_);
        size_t len  = 0;
        const char* src = lua_tolstring(ls_, -1, &len);
        param_.assign(src, len);
        lua_pop(ls_, 1);

        return 0;
    }
    static int lua2cpp(lua_State* ls_, int pos_, std::string& param_)
    {
        size_t len = 0;
        const char* str = luaL_checklstring(ls_, pos_, &len);
        param_.assign(str, len);
        return 0;
    }
};
template<> struct luacpp_op_t<float>
{
	static void cpp2luastack(lua_State* ls_, float arg_)
	{
		lua_pushnumber(ls_, (lua_Number)arg_);
	}
	static int luareturn2cpp(lua_State* ls_, int pos_, float& param_)
	{
		if (!lua_isnumber(ls_, pos_))
		{
			return -1;
		}
		param_ = (float)lua_tonumber(ls_, pos_);
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, float& param_)
	{
		param_ = (float)luaL_checknumber(ls_, pos_);
		return 0;
	}
};
template<> struct luacpp_op_t<double>
{
	static void cpp2luastack(lua_State* ls_, double arg_)
	{
		lua_pushnumber(ls_, (lua_Number)arg_);
	}
	static int luareturn2cpp(lua_State* ls_, int pos_, double& param_)
	{
		if (!lua_isnumber(ls_, pos_))
		{
			return -1;
		}
		param_ = (double)lua_tonumber(ls_, pos_);
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, double& param_)
	{
		param_ = (double)luaL_checknumber(ls_, pos_);
		return 0;
	}
};
template<> struct luacpp_op_t<void>
{
	static void cpp2luastack(lua_State* ls_, double arg_)
	{
		
	}
    
	static int luareturn2cpp(lua_State* ls_, int pos_, int& param_)
	{
		
		return 0;
	}
	static int lua2cpp(lua_State* ls_, int pos_, int& param_)
	{
		
		return 0;
	}
};
template<typename T>
struct luacpp_op_t<std::vector<T> >
{
    static void cpp2luastack(lua_State* ls_, const std::vector<T>& arg_)
    {
    	lua_newtable(ls_);
    	typename std::vector<T>::const_iterator it = arg_.begin();
    	for (int i = 1; it != arg_.end(); ++it, ++i)
    	{
    		luacpp_op_t<int>::cpp2luastack(ls_, i);
    		luacpp_op_t<T>::cpp2luastack(ls_, *it);
			lua_settable(ls_, -3);
    	}
    }

    static int luareturn2cpp(lua_State* ls_, int pos_, std::vector<T>& param_)
    {
    	if (0 == lua_istable(ls_, pos_))
    	{
    		return -1;
    	}
    	lua_pushnil(ls_);
    	int real_pos = pos_;
    	if (pos_ < 0) real_pos = real_pos - 1;

		while (lua_next(ls_, real_pos) != 0)
		{
			param_.push_back(T());
			if (luacpp_op_t<T>::luareturn2cpp(ls_, -1, param_[param_.size() - 1]) < 0)
			{
				return -1;
			}
			lua_pop(ls_, 1);
		}
       return 0;
    }

    static int lua2cpp(lua_State* ls_, int pos_, std::vector<T>& param_)
    {
    	luaL_checktype(ls_, pos_, LUA_TTABLE);

		lua_pushnil(ls_);
    	int real_pos = pos_;
    	if (pos_ < 0) real_pos = real_pos - 1;
		while (lua_next(ls_, real_pos) != 0)
		{
			param_.push_back(T());
			if (luacpp_op_t<T>::lua2cpp(ls_, -1, param_[param_.size() - 1]) < 0)
			{
				luaL_argerror(ls_, pos_>0?pos_:-pos_, "convert to vector failed");
			}
			lua_pop(ls_, 1);
		}
		return 0;
    }
};

struct lua_err_handler_t{
    
    static std::string luatraceback(lua_State* ls_, const char *fmt, ...)
    {
        std::string ret;
        char buff[1024];

        va_list argp;
        va_start(argp, fmt);
        vsnprintf(buff, sizeof(buff), fmt, argp);
        va_end(argp);

        ret = buff;
        snprintf(buff, sizeof(buff), " tracback:%s", lua_tostring(ls_, -1));
        ret += buff;

        return ret;
    }
};

class lua_err_t: public std::exception
{
public:
    explicit lua_err_t(const char* err_):
		m_err(err_)
	{}
    explicit lua_err_t(const std::string& err_):
		m_err(err_)
	{
	}
    ~lua_err_t() throw(){}

    const char* what() const throw () { return m_err.c_str(); }
private:
    std::string m_err;
};

template <typename FUNC_TYPE>
struct reg_func_traits_t;
template<typename ARG_TYPE>
struct cpp_type_traits_t;

template<typename ARG_TYPE>
struct cpp_type_traits_t
{
    typedef ARG_TYPE arg_type_t;
};
template<>
struct cpp_type_traits_t<const std::string&>
{
    typedef std::string arg_type_t;
};
template<typename T>
struct cpp_type_traits_t<const std::vector<T>&>
{
    typedef std::vector<T> arg_type_t;
};

template <typename T>
struct cpptype_init_traits_t
{
    inline static T value(){ return T(); }
};
template <>
struct cpptype_init_traits_t<std::string>
{
    inline static const char* value(){ return ""; }
};

template <>
struct cpptype_init_traits_t<const std::string&>
{
    inline static const char* value(){ return ""; }
};
template <typename T>
struct cpptype_init_traits_t<const std::vector<T>& >
{
    inline static std::vector<T> value(){ return std::vector<T>(); }
};


template <typename RET>
struct reg_func_traits_t<RET (*)()>
{
    typedef RET (*dest_func_t)();
    static  int lua_function(lua_State* ls_)
    {
        void* user_data = lua_touserdata (ls_, lua_upvalueindex(1));
        dest_func_t& registed_func = *((dest_func_t*)user_data);

        RET ret = registed_func();
        luacpp_op_t<RET>::cpp2luastack(ls_, ret);

        return 1;
    }
};

template<>
struct reg_func_traits_t<int(*)(lua_State*)>
{
    typedef int (*dest_func_t)(lua_State*);
    static  int lua_function(lua_State* ls_)
    {
        void* user_data = lua_touserdata (ls_, lua_upvalueindex(1));

        dest_func_t& registed_func = *((dest_func_t*)user_data);
        return registed_func(ls_);
    }
};

template <typename RET, typename ARG1>
struct reg_func_traits_t<RET (*)(ARG1)>
{
    typedef RET (*dest_func_t)(ARG1);
    static  int lua_function(lua_State* ls_)
    {
        typename cpp_type_traits_t<ARG1>::arg_type_t arg1 = cpptype_init_traits_t<ARG1>::value();
        luacpp_op_t<typename cpp_type_traits_t<ARG1>::arg_type_t>::lua2cpp(ls_, ADDR_ARG_POS(1), arg1);

        void* user_data = lua_touserdata (ls_, lua_upvalueindex(1));
        dest_func_t& registed_func = *((dest_func_t*)user_data);

        RET ret = registed_func(arg1);
        luacpp_op_t<RET>::cpp2luastack(ls_, ret);
        return 1;
    }
};

template <typename RET, typename ARG1, typename ARG2>
struct reg_func_traits_t<RET (*)(ARG1, ARG2)>
{
    typedef RET (*dest_func_t)(ARG1, ARG2);
    static  int lua_function(lua_State* ls_)
    {
        typename cpp_type_traits_t<ARG1>::arg_type_t arg1 = cpptype_init_traits_t<ARG1>::value();
        typename cpp_type_traits_t<ARG2>::arg_type_t arg2 = cpptype_init_traits_t<ARG2>::value();
        luacpp_op_t<typename cpp_type_traits_t<ARG1>::arg_type_t>::lua2cpp(ls_, ADDR_ARG_POS(1), arg1);
        luacpp_op_t<typename cpp_type_traits_t<ARG2>::arg_type_t>::lua2cpp(ls_, ADDR_ARG_POS(2), arg2);

        void* user_data = lua_touserdata (ls_, lua_upvalueindex(1));
        dest_func_t& registed_func = *((dest_func_t*)user_data);

        RET ret = registed_func(arg1, arg2);
        luacpp_op_t<RET>::cpp2luastack(ls_, ret);
        return 1;
    }
};

template <typename RET, typename ARG1, typename ARG2, typename ARG3>
struct reg_func_traits_t<RET (*)(ARG1, ARG2, ARG3)>
{
    typedef RET (*dest_func_t)(ARG1, ARG2, ARG3);
    static  int lua_function(lua_State* ls_)
    {
        typename cpp_type_traits_t<ARG1>::arg_type_t arg1 = cpptype_init_traits_t<ARG1>::value();
        typename cpp_type_traits_t<ARG2>::arg_type_t arg2 = cpptype_init_traits_t<ARG2>::value();
        typename cpp_type_traits_t<ARG3>::arg_type_t arg3 = cpptype_init_traits_t<ARG3>::value();

        luacpp_op_t<typename cpp_type_traits_t<ARG1>::arg_type_t>::lua2cpp(ls_, ADDR_ARG_POS(1), arg1);
        luacpp_op_t<typename cpp_type_traits_t<ARG2>::arg_type_t>::lua2cpp(ls_, ADDR_ARG_POS(2), arg2);
        luacpp_op_t<typename cpp_type_traits_t<ARG3>::arg_type_t>::lua2cpp(ls_, ADDR_ARG_POS(3), arg3);

        void* user_data = lua_touserdata (ls_, lua_upvalueindex(1));
        dest_func_t& registed_func = *((dest_func_t*)user_data);

        RET ret = registed_func(arg1, arg2, arg3);
        luacpp_op_t<RET>::cpp2luastack(ls_, ret);
        return 1;
    }
};

template <typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
struct reg_func_traits_t<RET (*)(ARG1, ARG2, ARG3, ARG4)>
{
    typedef RET (*dest_func_t)(ARG1, ARG2, ARG3, ARG4);
    static  int lua_function(lua_State* ls_)
    {
        typename cpp_type_traits_t<ARG1>::arg_type_t arg1 = cpptype_init_traits_t<ARG1>::value();
        typename cpp_type_traits_t<ARG2>::arg_type_t arg2 = cpptype_init_traits_t<ARG2>::value();
        typename cpp_type_traits_t<ARG3>::arg_type_t arg3 = cpptype_init_traits_t<ARG3>::value();
        typename cpp_type_traits_t<ARG4>::arg_type_t arg4 = cpptype_init_traits_t<ARG4>::value();

        luacpp_op_t<typename cpp_type_traits_t<ARG1>::arg_type_t>::lua2cpp(ls_, ADDR_ARG_POS(1), arg1);
        luacpp_op_t<typename cpp_type_traits_t<ARG2>::arg_type_t>::lua2cpp(ls_, ADDR_ARG_POS(2), arg2);
        luacpp_op_t<typename cpp_type_traits_t<ARG3>::arg_type_t>::lua2cpp(ls_, ADDR_ARG_POS(3), arg3);
        luacpp_op_t<typename cpp_type_traits_t<ARG4>::arg_type_t>::lua2cpp(ls_, ADDR_ARG_POS(4), arg4);

        void* user_data = lua_touserdata (ls_, lua_upvalueindex(1));
        dest_func_t& registed_func = *((dest_func_t*)user_data);

        RET ret = registed_func(arg1, arg2, arg3, arg4);
        luacpp_op_t<RET>::cpp2luastack(ls_, ret);
        return 1;
    }
};

class lua_reg_func_t
{
public:
	lua_reg_func_t(lua_State* ls_, const std::string& ext_name_):m_ls(ls_), m_ext_name(ext_name_){
        
        lua_newtable(ls_);

        lua_setglobal(ls_, m_ext_name.c_str());
    }
    template<typename FUNC>
	lua_reg_func_t& def(FUNC func_, const std::string& func_name_)
	{
		lua_func_t lua_func = reg_func_traits_t<FUNC>::lua_function;

		void* user_data_ptr = lua_newuserdata(m_ls, sizeof(func_));
		new(user_data_ptr) FUNC(func_);

		lua_pushcclosure(m_ls, lua_func, 1);
        if (m_ext_name.empty())
        {
            lua_setglobal(m_ls, func_name_.c_str());
        }
        else{
            lua_getglobal(m_ls, m_ext_name.c_str());
            lua_pushstring(m_ls, func_name_.c_str());
            lua_pushvalue(m_ls, -3);
            lua_settable(m_ls, -3);

            lua_pop(m_ls, 2);
        }
		return *this;
	}
private:
	lua_State* 	        m_ls;
	std::string 		m_ext_name;
};

template <typename T>
struct func_ret_type_traits{
    typedef T RET_TYPE;
    static int is_void(){
        return false;
    }
};
template <>
struct func_ret_type_traits<void>{
    typedef int RET_TYPE;
    static int is_void(){
        return true;
    }
};

struct lua_args_t{
    struct args_into_t{
        int64_t     n;
        std::string s;
        int         argtype;
    };
    lua_args_t& add(int64_t n){
        args_into_t arg;
        arg.n = n;
        arg.argtype = 0;
        args.push_back(arg);
        return *this;
    }
    lua_args_t& add(uint64_t n){
        args_into_t arg;
        arg.n = n;
        arg.argtype = 0;
        args.push_back(arg);
        return *this;
    }
    lua_args_t& add(const std::string& s){
        args_into_t arg;
        arg.s = s;
        arg.argtype = 1;
        args.push_back(arg);
        return *this;
    }
    void cpp2luastack(lua_State* ls_) const{
        for (unsigned int i = 0; i < args.size(); ++i){
            if (args[i].argtype == 0){
                lua_pushnumber(ls_, (lua_Number)args[i].n);
            }
            else{
                lua_pushlstring(ls_, args[i].s.c_str(), args[i].s.size());
            }
        }
        
    }
    std::vector<args_into_t> args;
};

struct luaops_t{

    template<typename RET>
    typename func_ret_type_traits<RET>::RET_TYPE call(const std::string& func_name_, const lua_args_t& luaarg){
        typedef typename func_ret_type_traits<RET>::RET_TYPE RET_V;
        
        RET_V ret = cpptype_init_traits_t<RET_V>::value();
        
        lua_getglobal(m_ls, func_name_.c_str());

        luaarg.cpp2luastack(m_ls);
        
        if (lua_pcall(m_ls, luaarg.args.size(), 1, 0) != 0)
        {
            std::string err = lua_err_handler_t::luatraceback(m_ls, "lua_pcall failed func_name<%s>", func_name_.c_str());
            lua_pop(m_ls, 1);
            throw lua_err_t(err);
        }

        if (luacpp_op_t<RET>::luareturn2cpp(m_ls, -1, ret))
        {
            lua_pop(m_ls, 1);
            char buff[512];
            snprintf(buff, sizeof(buff), "callfunc [arg1] luareturn2cpp failed  func_name<%s>", func_name_.c_str());
            throw lua_err_t(buff);
        }

        lua_pop(m_ls, 1);

        return ret;
    }
    luaops_t():
		m_ls(NULL)
	{
		m_ls = ::luaL_newstate();
		::luaL_openlibs(m_ls);
	}
    ~luaops_t()
    {
        if (m_ls)
        {
            ::lua_close(m_ls);
            m_ls = NULL;
        }
    }
    
    lua_State* get_lua_state()
    {
        return m_ls;
    }

    int  add_package_path(const std::string& str_)
    {
        std::string new_path = "package.path = package.path .. \"";
        if (str_.empty())
        {
            return -1;
        }

        if (str_[0] != ';')
        {
           new_path += ";";
        }

        new_path += str_;

        if (str_[str_.length() - 1] != '/')
        {
            new_path += "/";
        }

        new_path += "?.lua\" ";

        run_string(new_path);
        return 0;
    }
    int  load_file(const std::string& file_name_) throw (lua_err_t)
	{
		if (luaL_loadfile(m_ls, file_name_.c_str()))
		{
			std::string err = lua_err_handler_t::luatraceback(m_ls, "cannot load file<%s>", file_name_.c_str());
			::lua_pop(m_ls, 1);
			throw lua_err_t(err);
		}

		return 0;
	}
	int  do_file(const std::string& file_name_) throw (lua_err_t)
	{
		if (luaL_dofile(m_ls, file_name_.c_str()))
		{
			std::string err = lua_err_handler_t::luatraceback(m_ls, "cannot load file<%s>", file_name_.c_str());
			::lua_pop(m_ls, 1);
			throw lua_err_t(err);
		}

		return 0;
	}
    void run_string(const char* str_) throw (lua_err_t)
	{
		if (luaL_dostring(m_ls, str_))
		{
			std::string err = lua_err_handler_t::luatraceback(m_ls, "fflua_t::run_string ::lua_pcall faled str<%s>", str_);
			::lua_pop(m_ls, 1);
			throw lua_err_t(err);
		}
	}
    void run_string(const std::string& str_) throw (lua_err_t)
    {
        run_string(str_.c_str());
    }
    template<typename T>
    void  reg(T a){
        a(this->get_lua_state());
    }
    
    lua_State*  m_ls;
};



}
#endif

