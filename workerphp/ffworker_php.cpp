#include <php_embed.h>

#include "./ffworker_php.h"
#include "base/performance_daemon.h"
#include "server/http_mgr.h"
#include "server/script.h"

#include <stdio.h> 
using namespace ff;
//using namespace std;
#ifndef SUCCESS
#define SUCCESS 0
#endif
#ifndef FAIL
#define FAIL 1
#endif
struct php_wrap_t{
    static zval* vecstr2zval(const std::vector<std::string>& data){
        zval* retval = NULL;
        MAKE_STD_ZVAL(retval);
        array_init(retval);
        
        for (size_t i = 0; i < data.size(); ++i){
            add_next_index_stringl(retval, data[i].c_str(), data[i].size(), true);
        }
        return retval;
    }
    static bool array_add(zval* retval, const std::string& key, zval* val){
        add_assoc_zval_ex(retval, key.c_str(), key.size()+1, val);
        return true;
    }
    static bool array_add_str(zval* retval, const std::string& key, const std::string& val){
        add_assoc_stringl_ex(retval, (char*)key.c_str(), key.size()+1, (char*)val.c_str(), val.size(), true);
        return true;
    }
    static bool array_add_int(zval* retval, const std::string& key, int64_t val){
        add_assoc_long_ex(retval, (char*)key.c_str(), key.size()+1, (long)val);
        return true;
    }
    static bool array_append(zval* retval, zval* val){
        add_next_index_zval(retval, val);
        return true;
    }
    static zval* build_int(int64_t n){
        zval* retval = NULL;
        MAKE_STD_ZVAL(retval);
        ZVAL_LONG(retval, n);
        return retval;
    }
    static zval* build_str(const std::string& m){
        zval* retval = NULL;
        MAKE_STD_ZVAL(retval);
        ZVAL_STRINGL(retval, m.c_str(), m.size(), 1);
        return retval;
    }
    static zval* build_cpy(zval* fromval){
        zval* retval = NULL;
        MAKE_STD_ZVAL(retval);
        *retval = *fromval;
        zval_copy_ctor(retval);

        INIT_PZVAL(retval);
        return retval;
    }
};
extern "C"{ void zend_fetch_debug_backtrace(zval *return_value, int skip_last, int options, int limit TSRMLS_DC); }
class phpops_t{
public:
    phpops_t():global_array(NULL), global_instance(NULL){
        
        int argc = 1;
        char *argv[2] = { (char*)"h2workerphp", NULL };
        php_embed_init(argc, argv PTSRMLS_CC);
        
        MAKE_STD_ZVAL(global_array);
        array_init(global_array);
        
        MAKE_STD_ZVAL(global_instance);
    }
    ~phpops_t(){
        zval_ptr_dtor(&global_array);
        global_array = NULL;
        
        zval_ptr_dtor(&global_instance);
        global_instance = NULL;
    }
    
    int eval_string(const std::string& code){
        int status = SUCCESS;
        //PUSH_CTX();
        zend_first_try {
            status = zend_eval_string((char*)code.c_str(), NULL, (char*)"" TSRMLS_CC);
        } zend_catch {
            status = FAIL;
        } zend_end_try();

        //POP_CTX();

        return status;
    }
    int load(const std::string& filename){
        char buff[256] = {0};
        snprintf(buff, sizeof(buff), "include_once('%s');", filename.c_str());
        return eval_string(buff);
    }
    bool global_cache(const std::string& keyname, zval* funccb){
        zval *data = NULL;
        MAKE_STD_ZVAL(data);
        *data = *funccb;
        zval_copy_ctor(data);
        INIT_PZVAL(data);
        add_assoc_zval_ex(global_array, keyname.c_str(), keyname.size()+1, data);
        
        zval_ptr_dtor(&funccb);
        return true;
    }

    zval * call_function(const std::string& funcname, std::vector<zval*>* vec_params = NULL, bool need_ret = false){
        zval *rrv = NULL;
        int status = SUCCESS;
        zval **params = NULL;
        zend_uint count = 0;
        if (vec_params && vec_params->empty() == false){
            count = vec_params->size();
            params = &((*vec_params)[0]);
        }
        zend_try {
            // convert the function name to a zval
            zval *function_name = NULL;
            MAKE_STD_ZVAL(function_name);
            ZVAL_STRING(function_name, funcname.c_str(), 0);
            
            //printf("TTTTTT %s\n", funcname.c_str());

            zval *rv = NULL;
            MAKE_STD_ZVAL(rv);
            if(call_user_function(EG(function_table), NULL, function_name, rv,
                                count, params TSRMLS_CC) != SUCCESS)
            {
                LOGERROR((FFWORKER_PHP, "calling function %s failed1\n", funcname.c_str()));
                status = FAIL;
            }
            
            for(unsigned int i = 0; i < count; i++){
                  if(params[i]){
                      zval_ptr_dtor(&params[i]);
                  }
            }
            efree(function_name);

            if(status != FAIL){
                if (need_ret){
                    rrv = rv;
                }
                else{
                    zval_ptr_dtor(&rv);
                }
            }
            
        } zend_catch {
            if (PG(last_error_message)) {
                long errtype = PG(last_error_type);
                std::string errmsg = PG(last_error_message);
                std::string errfile = PG(last_error_file)?PG(last_error_file):"-";
                long errline = PG(last_error_lineno);
                
                LOGERROR((FFWORKER_PHP, "calling function %s errtype:%ld,%s:%ld errmsg:%s\n",
                                        funcname, errtype, errfile, errline, errmsg));
            }
            else{
                LOGERROR((FFWORKER_PHP, "calling function %s exception", funcname));
            }
            status = FAIL;
            
            // long options = DEBUG_BACKTRACE_PROVIDE_OBJECT;
            // long limit = 0;

            
            // //array_init(retval);
            // zend_fetch_debug_backtrace(retval, 1, options, limit TSRMLS_CC);
            // printf("zend_hash_num_elements %d\n", zend_hash_num_elements(Z_ARRVAL_P(retval)));
            // zval_ptr_dtor(&retval);
        } zend_end_try() {
        }

        if(status == FAIL){
            throw std::runtime_error("call func failed");
        }
        return rrv;
    }
    
    void call_phpcallback(const std::string& funcname){
         std::vector<zval*> params;
         params.push_back(php_wrap_t::build_cpy(global_array));
         params.push_back(php_wrap_t::build_str(funcname));
         call_function("h2ext_callback", &params);
         
         zend_hash_del(Z_ARRVAL_P(global_array), funcname.c_str(), funcname.size() + 1);
    }
    void call_phpcallback(const std::string& funcname, zval* arg){
         std::vector<zval*> params;
         params.push_back(php_wrap_t::build_cpy(global_array));
         params.push_back(php_wrap_t::build_str(funcname));
         params.push_back(arg);
         call_function("h2ext_callback1", &params);
         
         zend_hash_del(Z_ARRVAL_P(global_array), funcname.c_str(), funcname.size() + 1);
    }
    
    void call_void(const std::string& funcname, int64_t nval){
        std::vector<zval*> params;
        params.push_back(php_wrap_t::build_int(nval));
        call_function(funcname, &params);
    }
    void call_void(const std::string& funcname, int64_t nval, const std::string& data){
        std::vector<zval*> params;
        params.push_back(php_wrap_t::build_int(nval));
        params.push_back(php_wrap_t::build_str(data));
        call_function(funcname, &params);
    }
    void call_void(const std::string& funcname, int64_t nval, int64_t nval2, const std::string& data){
        std::vector<zval*> params;
        params.push_back(php_wrap_t::build_int(nval));
        params.push_back(php_wrap_t::build_int(nval2));
        params.push_back(php_wrap_t::build_str(data));
        call_function(funcname, &params);
    }
    std::string call_string(const std::string& funcname, int32_t cmd, const std::string& data){
        std::string strret;
        
        std::vector<zval*> params;
        params.push_back(php_wrap_t::build_int(cmd));
        params.push_back(php_wrap_t::build_str(data));
        zval* retval = call_function(funcname, &params, true);
        
        if (retval){
            if(Z_TYPE_P(retval) == IS_STRING)
            {
                strret.assign(Z_STRVAL_P(retval), Z_STRLEN_P(retval));
            }
            zval_ptr_dtor(&retval);
        }
        return strret;
    }
    //php_stl     phpstl;
    zval*       global_array;
    zval*       global_instance;
};

FFWorkerPhp::FFWorkerPhp():
    m_enable_call(true), m_started(false)
{
    m_php = new phpops_t();
}
FFWorkerPhp::~FFWorkerPhp()
{
}

static zend_class_entry *h2ext_class_entry = NULL;

PHP_FUNCTION(confirm_h2ext_compiled)
{
	const char *strg = "";
	RETURN_STRINGL(strg, 0, 0);
}

// PHP_FUNCTION(h2ext_inst)
// {
	// zval* retval = Singleton<FFWorkerPhp>::instance().m_php->global_instance;
    // RETURN_ZVAL(retval, 0, 1);
// }

zend_function_entry h2ext_functions[] = {
	PHP_FE(confirm_h2ext_compiled,	NULL)
    //PHP_FE(h2ext_inst,	NULL)
	{NULL, NULL, NULL}
};


PHP_METHOD(h2ext, __construct)
{
    RETURN_TRUE;    
}

PHP_METHOD(h2ext, sessionSendMsg)
{
    userid_t session_id_ = 0;
    long cmd_ = 0;
	char *strarg  = NULL;
    long strlen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &session_id_, &cmd_, &strarg, &strlen) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string data_(strarg, strlen);
    Singleton<FFWorkerPhp>::instance().sessionSendMsg(session_id_, cmd_, data_);
    RETURN_TRUE;
    
}
PHP_METHOD(h2ext, gateBroadcastMsg)
{
    long cmd_ = 0;
	char *strarg  = NULL;
    long strlen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &cmd_, &strarg, &strlen) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string data_(strarg, strlen);
    Singleton<FFWorkerPhp>::instance().gateBroadcastMsg(cmd_, data_);
    RETURN_TRUE;
}
PHP_METHOD(h2ext, sessionMulticastMsg)
{
    zval* ids = NULL;
    long cmd_ = 0;
	char *strarg  = NULL;
    long strlen = 0;
    std::vector<userid_t> session_ids;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "als", &ids, &cmd_, &strarg, &strlen) == FAILURE) {
		RETURN_FALSE;
	}
    
    if(Z_TYPE_P(ids) != IS_ARRAY){
        RETURN_FALSE;
    }
    ::zend_hash_internal_pointer_reset(Z_ARRVAL_P(ids));
    
    zval **data = NULL;
    
    while (::zend_hash_get_current_data(Z_ARRVAL_P(ids), (void **)&data) == SUCCESS){
        if (Z_TYPE_PP(data) != IS_LONG){
            convert_to_long_ex(data);
        }

        userid_t id = Z_LVAL_PP(data);
        session_ids.push_back(id);
        zend_hash_move_forward(Z_ARRVAL_P(ids));
        
        //LOGINFO((FFWORKER_PHP, "trace %d", id));
    }
    //!zval_ptr_dtor(&ids);
    
    std::string data_(strarg, strlen);
    Singleton<FFWorkerPhp>::instance().sessionMulticastMsg(session_ids, cmd_, data_);
    RETURN_TRUE;
}
PHP_METHOD(h2ext, sessionClose)
{
    userid_t session_id_ = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &session_id_) == FAILURE) {
		RETURN_FALSE;
	}
    
    Singleton<FFWorkerPhp>::instance().sessionClose(session_id_);
    RETURN_TRUE;
}

PHP_METHOD(h2ext, sessionChangeWorker)
{
    userid_t session_id_ = 0;
    long to_worker_index_ = 0;
	char *strarg  = NULL;
    long strlen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lls", &session_id_, &to_worker_index_, &strarg, &strlen) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string extra_data(strarg, strlen);
    Singleton<FFWorkerPhp>::instance().sessionChangeWorker(session_id_, to_worker_index_, extra_data);
    RETURN_TRUE;
}
PHP_METHOD(h2ext, getSessionGate)
{
    userid_t session_id_ = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &session_id_) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string ret = Singleton<FFWorkerPhp>::instance().getSessionGate(session_id_);
    RETURN_STRINGL(ret.c_str(), ret.size(), 1);
}
PHP_METHOD(h2ext, getSessionIp)
{
    userid_t session_id_ = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &session_id_) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string ret = Singleton<FFWorkerPhp>::instance().getSessionIp(session_id_);
    RETURN_STRINGL(ret.c_str(), ret.size(), 1);
}
PHP_METHOD(h2ext, isExist)
{
	char *strarg  = NULL;
    long strlen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &strarg, &strlen) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string service_name_(strarg, strlen);
    bool ret = Singleton<FFWorkerPhp>::instance().getRpc().isExist(service_name_);
    RETURN_BOOL(ret);
}
PHP_METHOD(h2ext, reload)
{
	char *strarg  = NULL;
    long strlen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &strarg, &strlen) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string name_(strarg, strlen);
    std::string ret = Singleton<FFWorkerPhp>::instance().reload(name_);
    RETURN_STRINGL(ret.c_str(), ret.size(), 1);
}
PHP_METHOD(h2ext, log)
{
    long level = 0;
	char *strarg  = NULL;
    long strlen = 0;
    char *strarg2  = NULL;
    long strlen2 = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lss", &level, &strarg, &strlen, &strarg2, &strlen2) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string mod_(strarg, strlen);
    std::string content_(strarg2, strlen2);
    Singleton<FFWorkerPhp>::instance().pylog(level, mod_, content_);
    RETURN_TRUE;
}
PHP_METHOD(h2ext, regTimer)
{
    static long timer_idx = 0;
    
    char fieldname[256] = {0};
    long idx = ++timer_idx;
    snprintf(fieldname, sizeof(fieldname), "timer#%ld", idx);
    
    
    long mstimeout_ = 0;
    zval* funccb = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &mstimeout_, &funccb) == FAILURE || funccb == NULL) {
		RETURN_FALSE;
	}

    Singleton<FFWorkerPhp>::instance().m_php->global_cache(fieldname, funccb);

    struct lambda_cb
    {
        static void callback(long idx)
        {
            char fieldname[256] = {0};
            snprintf(fieldname, sizeof(fieldname), "timer#%ld", idx);
 
            try
            {
                Singleton<FFWorkerPhp>::instance().m_php->call_phpcallback(fieldname);
            }
            catch(std::exception& e_)
            {
                LOGERROR((FFWORKER_PHP, "FFWorkerPhp::regTimer std::exception<%s>", e_.what()));
            }
            
        }
    };
    
    Singleton<FFWorkerPhp>::instance().regTimer(mstimeout_, 
                TaskBinder::gen(&lambda_cb::callback, idx));
    RETURN_TRUE;
}
PHP_METHOD(h2ext, writeLockGuard)
{
    Singleton<FFWorkerPhp>::instance().getSharedMem().writeLockGuard();
    RETURN_TRUE;
}
PHP_METHOD(h2ext, connectDB)
{
	char *strarg  = NULL;
    long strlen = 0;

    char *strarg2  = NULL;
    long strlen2 = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &strarg, &strlen, &strarg2, &strlen2) == FAILURE) {
		RETURN_FALSE;
	}
    std::string host_(strarg, strlen);
    std::string group_(strarg2, strlen2);
    long ret = DB_MGR_OBJ.connectDB(host_, group_);
    
    RETURN_LONG(ret);
}


PHP_METHOD(h2ext, asyncQuery)
{
    long db_id_ = 0;
	char *strarg  = NULL;
    long strlen = 0;
    zval* funccb = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsz", &db_id_, &strarg, &strlen, &funccb) == FAILURE || funccb == NULL) {
		RETURN_FALSE;
	}
    
    std::string sql_(strarg, strlen);
    
    static int64_t db_idx = 0;
    char fieldname[256] = {0};
    ++db_idx;
    long idx = long(db_idx);
    snprintf(fieldname, sizeof(fieldname), "db#%ld", idx);
    Singleton<FFWorkerPhp>::instance().m_php->global_cache(fieldname, funccb);
    
    struct lambda_cb: public FFSlot::FFCallBack
    {
        lambda_cb(long idxarg):idx(idxarg){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(DbMgr::queryDBResult_t))
            {
                return;
            }
            DbMgr::queryDBResult_t* data = (DbMgr::queryDBResult_t*)args_;

            Singleton<FFWorkerPhp>::instance().getRpc().get_tq().produce(TaskBinder::gen(&lambda_cb::call_php, idx,
                                                                   data->errinfo, data->result_data, data->col_names, data->affectedRows));
        }
        static void call_php(long idx, std::string errinfo, std::vector<std::vector<std::string> > ret_, std::vector<std::string> col_, int affectedRows)
        {
            if (Singleton<FFWorkerPhp>::instance().m_enable_call == false)
            {
                return;
            }
            char fieldname[256] = {0};
            snprintf(fieldname, sizeof(fieldname), "db#%ld", idx);
            
            zval* retval = NULL;
            MAKE_STD_ZVAL(retval);
            array_init(retval);

            {
                std::string key = "datas";
                
                zval* val_arr = NULL;
                MAKE_STD_ZVAL(val_arr);
                array_init(val_arr);
                
                for (size_t i = 0; i < ret_.size(); ++i){
                    php_wrap_t::array_append(val_arr, php_wrap_t::vecstr2zval(ret_[i]));
                }
                php_wrap_t::array_add(retval, key, val_arr);
            }
            
            {
                std::string key = "fields";
                php_wrap_t::array_add(retval, key, php_wrap_t::vecstr2zval(col_));
            }
            {
                std::string key = "errinfo";
                php_wrap_t::array_add_str(retval, key, errinfo);
            }
            {
                std::string key = "affectedRows";
                php_wrap_t::array_add_int(retval, key, affectedRows);
            }
            Singleton<FFWorkerPhp>::instance().m_php->call_phpcallback(fieldname, retval);
            zval_ptr_dtor(&retval);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(idx); }
        long idx;
    };
    
    DB_MGR_OBJ.queryDB(db_id_, sql_,  new lambda_cb(idx));
    RETURN_TRUE;
}

PHP_METHOD(h2ext, asyncQueryGroupMod)
{
    char *strgroup  = NULL;
    long lengroup = 0;
    
    long mod_ = 0;
	char *strarg  = NULL;
    long strlen = 0;
    zval* funccb = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slsz", &strgroup, &lengroup, &mod_, &strarg, &strlen, &funccb) == FAILURE || funccb == NULL) {
		RETURN_FALSE;
	}
    
    std::string group_(strgroup, lengroup);
    std::string sql_(strarg, strlen);
    
    static int64_t db_idx = 0;
    char fieldname[256] = {0};
    ++db_idx;
    long idx = long(db_idx);
    snprintf(fieldname, sizeof(fieldname), "db#%ld", idx);
    Singleton<FFWorkerPhp>::instance().m_php->global_cache(fieldname, funccb);
    
    struct lambda_cb: public FFSlot::FFCallBack
    {
        lambda_cb(long idxarg):idx(idxarg){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(DbMgr::queryDBResult_t))
            {
                return;
            }
            DbMgr::queryDBResult_t* data = (DbMgr::queryDBResult_t*)args_;

            Singleton<FFWorkerPhp>::instance().getRpc().get_tq().produce(TaskBinder::gen(&lambda_cb::call_php, idx,
                                                                   data->errinfo, data->result_data, data->col_names, data->affectedRows));
        }
        static void call_php(long idx, std::string errinfo, std::vector<std::vector<std::string> > ret_, std::vector<std::string> col_, int affectedRows)
        {
            if (Singleton<FFWorkerPhp>::instance().m_enable_call == false)
            {
                return;
            }
            char fieldname[256] = {0};
            snprintf(fieldname, sizeof(fieldname), "db#%ld", idx);
            
            zval* retval = NULL;
            MAKE_STD_ZVAL(retval);
            array_init(retval);

            {
                std::string key = "datas";
                
                zval* val_arr = NULL;
                MAKE_STD_ZVAL(val_arr);
                array_init(val_arr);
                
                for (size_t i = 0; i < ret_.size(); ++i){
                    php_wrap_t::array_append(val_arr, php_wrap_t::vecstr2zval(ret_[i]));
                }
                php_wrap_t::array_add(retval, key, val_arr);
            }
            
            {
                std::string key = "fields";
                php_wrap_t::array_add(retval, key, php_wrap_t::vecstr2zval(col_));
            }
            {
                std::string key = "errinfo";
                php_wrap_t::array_add_str(retval, key, errinfo);
            }
            {
                std::string key = "affectedRows";
                php_wrap_t::array_add_int(retval, key, affectedRows);
            }
            Singleton<FFWorkerPhp>::instance().m_php->call_phpcallback(fieldname, retval);
            zval_ptr_dtor(&retval);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(idx); }
        long idx;
    };
    
    DB_MGR_OBJ.queryDBGroupMod(group_, mod_, sql_,  new lambda_cb(idx));
    RETURN_TRUE;
}
PHP_METHOD(h2ext, query)
{
    long db_id_ = 0;
	char *strarg  = NULL;
    long strlen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &db_id_, &strarg, &strlen) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string sql_(strarg, strlen);
    
    std::string errinfo;
    std::vector<std::vector<std::string> > ret_;
    std::vector<std::string> col_;
    int affectedRows = 0;
    DB_MGR_OBJ.syncQueryDB(db_id_, sql_, ret_, col_, errinfo, affectedRows);
    
    zval* retval = NULL;
    MAKE_STD_ZVAL(retval);
    array_init(retval);

    {
        std::string key = "datas";
        
        zval* val_arr = NULL;
        MAKE_STD_ZVAL(val_arr);
        array_init(val_arr);
        
        for (size_t i = 0; i < ret_.size(); ++i){
            php_wrap_t::array_append(val_arr, php_wrap_t::vecstr2zval(ret_[i]));
        }
        php_wrap_t::array_add(retval, key, val_arr);
    }
    
    {
        std::string key = "fields";
        php_wrap_t::array_add(retval, key, php_wrap_t::vecstr2zval(col_));
    }
    {
        std::string key = "errinfo";
        php_wrap_t::array_add_str(retval, key, errinfo);
    }
    {
        std::string key = "affectedRows";
        php_wrap_t::array_add_int(retval, key, affectedRows);
    }
    RETURN_ZVAL(retval, 0, 1);
}
PHP_METHOD(h2ext, queryGroupMod)
{
    char *strgroup  = NULL;
    long lengroup = 0;
    
    long mod_ = 0;
	char *strarg  = NULL;
    long strlen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sls", &strgroup, &lengroup, &mod_, &strarg, &strlen) == FAILURE) {
		RETURN_FALSE;
	}
    std::string group_(strgroup, lengroup);
    std::string sql_(strarg, strlen);
    
    std::string errinfo;
    std::vector<std::vector<std::string> > ret_;
    std::vector<std::string> col_;
    int affectedRows = 0;
    DB_MGR_OBJ.syncQueryDBGroupMod(group_, mod_, sql_, ret_, col_, errinfo, affectedRows);
    
    zval* retval = NULL;
    MAKE_STD_ZVAL(retval);
    array_init(retval);

    {
        std::string key = "datas";
        
        zval* val_arr = NULL;
        MAKE_STD_ZVAL(val_arr);
        array_init(val_arr);
        
        for (size_t i = 0; i < ret_.size(); ++i){
            php_wrap_t::array_append(val_arr, php_wrap_t::vecstr2zval(ret_[i]));
        }
        php_wrap_t::array_add(retval, key, val_arr);
    }
    
    {
        std::string key = "fields";
        php_wrap_t::array_add(retval, key, php_wrap_t::vecstr2zval(col_));
    }
    {
        std::string key = "errinfo";
        php_wrap_t::array_add_str(retval, key, errinfo);
    }
    {
        std::string key = "affectedRows";
        php_wrap_t::array_add_int(retval, key, affectedRows);
    }
    RETURN_ZVAL(retval, 0, 1);
}
PHP_METHOD(h2ext, workerRPC)
{
    long workerindex = 0;
    long cmd = 0;
	char *strarg  = NULL;
    long strlen = 0;
    zval* funccb = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llsz", &workerindex, &cmd, &strarg, &strlen, &funccb) == FAILURE || funccb == NULL) {
		RETURN_FALSE;
	}
    
    std::string argdata(strarg, strlen);
    
    static int64_t rpc_idx = 0;
    char fieldname[256] = {0};
    ++rpc_idx;
    long idx = long(rpc_idx);
    snprintf(fieldname, sizeof(fieldname), "rpc#%ld", idx);
    Singleton<FFWorkerPhp>::instance().m_php->global_cache(fieldname, funccb);
    
    struct lambda_cb: public FFSlot::FFCallBack
    {
        lambda_cb(int idxarg):idx(idxarg){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(ffslot_req_arg))
            {
                return;
            }
            ffslot_req_arg* msg_data = (ffslot_req_arg*)args_;
            WorkerCallMsgt::out_t retmsg;
            try{
                ffthrift_t::DecodeFromString(retmsg, msg_data->body);
            }
            catch(std::exception& e_)
            {
            }
            
            char fieldname[256] = {0};
            snprintf(fieldname, sizeof(fieldname), "rpc#%ld", idx);
            
            try
            {
                zval* func_arg = php_wrap_t::build_str(retmsg.body);
                Singleton<FFWorkerPhp>::instance().m_php->call_phpcallback(fieldname, func_arg);
            }
            catch(std::exception& e_)
            {
                LOGERROR((FFWORKER_PHP, "FFWorkerPhp::call_service std::exception=%s", e_.what()));
            }
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(idx); }
        long idx;
    };
    Singleton<FFWorkerPhp>::instance().workerRPC(workerindex, (uint16_t)cmd, argdata, new lambda_cb(idx));
    
    RETURN_TRUE;
}
PHP_METHOD(h2ext, syncSharedData)
{
    long cmd_ = 0;
	char *strarg  = NULL;
    long strlen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &cmd_, &strarg, &strlen) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string data_(strarg, strlen);
    Singleton<FFWorkerPhp>::instance().getSharedMem().syncSharedData(cmd_, data_);
    RETURN_TRUE;
}
PHP_METHOD(h2ext, asyncHttp)
{
    long timeoutsec = 0;
	char *strarg  = NULL;
    long strlen = 0;
    zval* funccb = NULL;
    
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slz", &strarg, &strlen, &timeoutsec, &funccb) == FAILURE || funccb == NULL) {
		RETURN_FALSE;
	}
    
    std::string url(strarg, strlen);
    
    static int64_t http_idx = 0;
    char fieldname[256] = {0};
    ++http_idx;
    long idx = long(http_idx);
    snprintf(fieldname, sizeof(fieldname), "http#%ld", idx);
    Singleton<FFWorkerPhp>::instance().m_php->global_cache(fieldname, funccb);
    
    struct lambda_cb: public FFSlot::FFCallBack
    {
        lambda_cb(long idxarg):idx(idxarg){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(HttpMgr::http_result_t))
            {
                return;
            }
            HttpMgr::http_result_t* data = (HttpMgr::http_result_t*)args_;

            Singleton<FFWorkerPhp>::instance().getRpc().get_tq().produce(TaskBinder::gen(&lambda_cb::call_php, idx, data->ret));
        }
        static void call_php(long idx, std::string retdata)
        {
            if (Singleton<FFWorkerPhp>::instance().m_enable_call == false)
            {
                return;
            }
            char fieldname[256] = {0};
            snprintf(fieldname, sizeof(fieldname), "http#%ld", idx);
            
            try
            {
                zval* func_arg = php_wrap_t::build_str(retdata);
                Singleton<FFWorkerPhp>::instance().m_php->call_phpcallback(fieldname, func_arg);
            }
            catch(std::exception& e_)
            {
                LOGERROR((FFWORKER_PHP, "workerobj_python_t::gen_queryDB_callback std::exception<%s>", e_.what()));
            }
            
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(idx); }
        long idx;
    };
    
    Singleton<FFWorkerPhp>::instance().asyncHttp(url, timeoutsec,  new lambda_cb(idx));
    RETURN_TRUE;
}
PHP_METHOD(h2ext, syncHttp)
{
    long timeoutsec = 0;
	char *strarg  = NULL;
    long strlen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &strarg, &strlen, &timeoutsec) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string url(strarg, strlen);
    std::string ret = Singleton<FFWorkerPhp>::instance().syncHttp(url, timeoutsec);
    RETURN_STRINGL(ret.c_str(), ret.size(), 1);
}
PHP_METHOD(h2ext, escape)
{
	char *strarg  = NULL;
    long strlen = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &strarg, &strlen) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string data_(strarg, strlen);
    std::string ret = FFDb::escape(data_);
    RETURN_STRINGL(ret.c_str(), ret.size(), 1);
}

static void when_syncSharedData(int32_t cmd, const std::string& data){
    try{
        Singleton<FFWorkerPhp>::instance().m_php->call_void("when_syncSharedData", cmd, data);
    }
    catch(std::exception& e_)
    {
        LOGERROR((FFWORKER_PHP, "FFWorkerPhp::when_syncSharedData std::exception=%s", e_.what()));
    }
}
static ScriptArgObjPtr toScriptArg(zval* pvalue_){
    ScriptArgObjPtr ret = new ScriptArgObj();

    if (Z_TYPE_P(pvalue_) == IS_BOOL){
        int n = Z_BVAL_P(pvalue_)? 1: 0;
        ret->toInt(n);
    }
    else if (Z_TYPE_P(pvalue_) == IS_LONG){
        ret->toInt(Z_LVAL_P(pvalue_));
    }
    else if (Z_TYPE_P(pvalue_) == IS_DOUBLE){
        ret->toFloat(Z_DVAL_P(pvalue_));
    }
    else if (Z_TYPE_P(pvalue_) == IS_STRING){
        std::string sval(Z_STRVAL_P(pvalue_), Z_STRLEN_P(pvalue_));
        ret->toString(sval);
    }
    else if (Z_TYPE_P(pvalue_) == IS_ARRAY){
        ret->toList();
        HashTable *ht_data = Z_ARRVAL_P(pvalue_);
        zend_hash_internal_pointer_reset(ht_data);
        zend_ulong num_key = 0;
        int retCode = 0;
        
        char *s_key = NULL;
        
        long nIndex = 0;
        while ( HASH_KEY_NON_EXISTANT != (retCode = zend_hash_get_current_key(ht_data, &s_key, &num_key, 0)) ) {
            //LOGERROR((FFWORKER_PHP, "FFWorkerPhp %d", num_key));
            if (HASH_KEY_IS_STRING == retCode){
                ret->toDict();
                break;
            }
            else{
                if (nIndex != (long)num_key){
                    ret->toDict();
                    break;
                }
            }
            zend_hash_move_forward(ht_data);
            ++nIndex;
        }
        
        zend_hash_internal_pointer_reset(ht_data);
        std::string strKey;
        while ( HASH_KEY_NON_EXISTANT != (retCode = zend_hash_get_current_key(ht_data, &s_key, &num_key, 0)) ) {
            zval **pdata = NULL;
            zend_hash_get_current_data(ht_data, (void**)&pdata);
            
            strKey.clear();
            if (HASH_KEY_IS_STRING == retCode){
                strKey = s_key;
            }
            else{
                char buff[64] = {0};
                snprintf(buff, sizeof(buff), "%ld", long(num_key));
                strKey = buff;
            }
            if (ret->isDict())
            {
                ret->dictVal[strKey] = toScriptArg(*pdata);
            }
            else if (ret->isList()){
                ret->listVal.push_back(toScriptArg(*pdata));
            }
            zend_hash_move_forward(ht_data);
        }
    }  

    return ret;
}

static zval* fromScriptArgToScript(ScriptArgObjPtr pvalue){
    zval  *ret = NULL;
    MAKE_STD_ZVAL(ret);
    if (!pvalue){
        ZVAL_NULL(ret);
    }
    if (pvalue->isNull()){
        ZVAL_NULL(ret);
    }
    else if (pvalue->isInt()){
        ZVAL_LONG(ret, pvalue->getInt());
    }
    else if (pvalue->isFloat()){
        ZVAL_DOUBLE(ret, pvalue->getFloat());
    }
    else if (pvalue->isString()){
        ZVAL_STRINGL(ret, pvalue->getString().c_str(), pvalue->getString().size(), 1);
    }
    else if (pvalue->isList()){
        array_init(ret);
        for (size_t i = 0; i < pvalue->listVal.size(); ++i){
            php_wrap_t::array_append(ret, fromScriptArgToScript(pvalue->listVal[i]));
        }
    }
    else if (pvalue->isDict()){
        array_init(ret);
        std::map<std::string, SharedPtr<ScriptArgObj> >::iterator it = pvalue->dictVal.begin();

        for (; it != pvalue->dictVal.end(); ++it)
        {
            php_wrap_t::array_add(ret, it->first, fromScriptArgToScript(it->second));
        }
    }
    return ret;
}
PHP_METHOD(h2ext, callFunc)
{
	char *strarg  = NULL;
    long strlen = 0;

    zval* args[9];
    memset(args, sizeof(args), 0);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|zzzzzzzzz", &strarg, &strlen,
        &args[0], &args[1], &args[2],
        &args[3], &args[4], &args[5],
        &args[6], &args[7], &args[8]) == FAILURE) {
		RETURN_FALSE;
	}
    
    std::string funcName(strarg, strlen);
    
    ScriptArgs scriptArgs;
    for (unsigned int i = 0; i < sizeof(args)/sizeof(zval*); ++i)
    {
        if (!args[i])
            break;
        ScriptArgObjPtr elem = toScriptArg(args[i]);
        scriptArgs.args.push_back(elem);
    }

    LOGTRACE((FFWORKER_PHP, "callFunc begin funcName=%s,argsize=%d", funcName, scriptArgs.args.size()));
    if (false == SCRIPT_UTIL.callFunc(funcName, scriptArgs)){
        LOGERROR((FFWORKER_PHP, "callFunc no funcname:%s", funcName));
        RETURN_FALSE;
    }
    
    zval* ret = fromScriptArgToScript(scriptArgs.getReturnValue());
    RETURN_ZVAL(ret, 0, 1);
}

static bool callScriptImpl(const std::string& funcName, ScriptArgs& varScript){
    if (!Singleton<FFWorkerPhp>::instance().m_enable_call)
    {
        return false;
    }
    
    std::string exceptInfo;
    try{
        std::vector<zval*> params;
        for (size_t i = 0; i < varScript.args.size(); ++i){
            params.push_back(fromScriptArgToScript(varScript.at(i)));
        }
        zval* retval = Singleton<FFWorkerPhp>::instance().m_php->call_function(funcName, &params, true);
        if (retval){
            varScript.ret = toScriptArg(retval);
            zval_ptr_dtor(&retval);
        }
    }
    catch(std::exception& e_)
    {
        LOGERROR((FFWORKER_PHP, "FFWorkerPhp::callScriptImpl exception=%s", e_.what()));
        exceptInfo = e_.what();
    }
    if (exceptInfo.empty() == false && SCRIPT_UTIL.isExceptEnable()){ 
        throw std::runtime_error(exceptInfo);
    }
    return true;
}

zend_function_entry h2ext_class_functions[] = {
    PHP_ME(h2ext, __construct, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, sessionSendMsg, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, gateBroadcastMsg, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, sessionMulticastMsg, NULL, ZEND_ACC_STATIC)
    
    PHP_ME(h2ext, escape, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, sessionClose, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, sessionChangeWorker, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, getSessionGate, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, getSessionIp, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, isExist, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, log, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, regTimer, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, connectDB, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, asyncQuery, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, query, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, asyncQueryGroupMod, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, queryGroupMod, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, workerRPC, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, syncSharedData, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, asyncHttp, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, syncHttp, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, writeLockGuard, NULL, ZEND_ACC_STATIC)
    PHP_ME(h2ext, callFunc, NULL, ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

PHP_MINIT_FUNCTION(h2ext)
{
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "h2ext", h2ext_class_functions);
    h2ext_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(h2ext)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(h2ext)
{
	return SUCCESS;
}
PHP_RSHUTDOWN_FUNCTION(h2ext)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(h2ext)
{
}

zend_module_entry php_mymod_entry = {
    STANDARD_MODULE_HEADER,
    EXT_NAME,
    h2ext_functions,
	PHP_MINIT(h2ext),
	PHP_MSHUTDOWN(h2ext),
	PHP_RINIT(h2ext),
	PHP_RSHUTDOWN(h2ext),
	PHP_MINFO(h2ext),
    "1.0",
    STANDARD_MODULE_PROPERTIES
};


int FFWorkerPhp::scriptInit(const std::string& root)
{
    LOGINFO((FFWORKER_PHP, "FFWorkerPhp::scriptInit begin %s", root));
    std::string path;
    std::size_t pos = root.find_last_of("/");
    if (pos != std::string::npos)
    {
        path = root.substr(0, pos+1);
        m_ext_name = root.substr(pos+1, root.size() - pos - 1);
    }
    else{
        m_ext_name = root;
    }
    
    LOGTRACE((FFWORKER_PHP, "FFWorkerPhp::scriptInit begin path:%s, m_ext_name:%s", path, m_ext_name));

    getSharedMem().setNotifyFunc(when_syncSharedData);

    DB_MGR_OBJ.start();
    
    int ret = -2;
    
    try{
        
        Mutex                    mutex;
        ConditionVar            cond(mutex);
        
        LockGuard lock(mutex);
        getRpc().get_tq().produce(TaskBinder::gen(&FFWorkerPhp::processInit, this, &cond, &ret));
        while (ret == -2){
            cond.time_wait(100);
        }
        
        if (ret < 0)
        {
            this->close();
            return -1;
        }
    }
    catch(std::exception& e_)
    {
        return -1;
    }
    m_started = true;
    LOGINFO((FFWORKER_PHP, "FFWorkerPhp::scriptInit end ok"));
    return ret;
}
//!!处理初始化逻辑
int FFWorkerPhp::processInit(ConditionVar* var, int* ret)
{
    try{
        zend_startup_module(&php_mymod_entry);
        char buff[256] = {0};
        snprintf(buff, sizeof(buff), "function h2ext_callback($func_array, $func_key){ return $func_array[$func_key](); }");
        m_php->eval_string(buff);
        snprintf(buff, sizeof(buff), "function h2ext_callback1($func_array, $func_key, $arg1){ return $func_array[$func_key]($arg1); }");
        m_php->eval_string(buff);
        
        // snprintf(buff, sizeof(buff), "function __BuildObj__(){ $ret = new h2ext(); $GLOBALS['%s'] = $ret; return $ret;}", EXT_NAME);
        // m_php->eval_string(buff);
        
        // zval* retval = m_php->call_function("__BuildObj__", NULL, true);
        
        // if (retval){
            // *(m_php->global_instance) = *retval;
            // zval_copy_ctor(m_php->global_instance);
            
            // zval_ptr_dtor(&retval);
        // }
        
        
        if(SUCCESS != m_php->load(m_ext_name.c_str())){
            printf("load failed\n");
            *ret = -1;
        }
        else{
            SCRIPT_UTIL.setCallScriptFunc(callScriptImpl);
            this->initModule();
                
            Singleton<FFWorkerPhp>::instance().m_php->call_function("init");
            printf("load ok\n");
            *ret = 0;
        }
    }
    catch(std::exception& e_)
    {
        *ret = -1;
        LOGERROR((FFWORKER_PHP, "FFWorkerPhp::open failed er=<%s>", e_.what()));
    }
    var->signal();
    return 0;
}
void FFWorkerPhp::scriptCleanup()
{
    try
    {
        Singleton<FFWorkerPhp>::instance().m_php->call_function("cleanup");
    }
    catch(std::exception& e_)
    {
        LOGERROR((FFWORKER_PHP, "scriptCleanup failed er=<%s>", e_.what()));
    }
    this->cleanupModule();
    LOGINFO((FFWORKER_PHP, "scriptCleanup end"));
    m_enable_call = false;
    DB_MGR_OBJ.stop();
}
int FFWorkerPhp::close()
{
    LOGINFO((FFWORKER_PHP, "close begin"));
    getRpc().get_tq().produce(TaskBinder::gen(&FFWorkerPhp::scriptCleanup, this));
    
    FFWorker::close();
    if (false == m_started)
        return 0;
    m_started = false;

    
    LOGINFO((FFWORKER_PHP, "close end"));
    return 0;
}

std::string FFWorkerPhp::reload(const std::string& name_)
{
    AUTO_PERF();
    LOGTRACE((FFWORKER_PHP, "FFWorkerPhp::reload begin name_[%s]", name_));
    return "not supported";
}
void FFWorkerPhp::pylog(int level_, const std::string& mod_, const std::string& content_)
{
    switch (level_)
    {
        case 1:
        {
            LOGFATAL((mod_.c_str(), "%s", content_));
        }
        break;
        case 2:
        {
            LOGERROR((mod_.c_str(), "%s", content_));
        }
        break;
        case 3:
        {
            LOGWARN((mod_.c_str(), "%s", content_));
        }
        break;
        case 4:
        {
            LOGINFO((mod_.c_str(), "%s", content_));
        }
        break;
        case 5:
        {
            LOGDEBUG((mod_.c_str(), "%s", content_));
        }
        break;
        case 6:
        {
            LOGTRACE((mod_.c_str(), "%s", content_));
        }
        break;
        default:
        {
            LOGTRACE((mod_.c_str(), "%s", content_));
        }
        break;
    }
}

int FFWorkerPhp::onSessionReq(userid_t session_id_, uint16_t cmd_, const std::string& data_)
{
    try
    {
        Singleton<FFWorkerPhp>::instance().m_php->call_void("onSessionReq", session_id_, cmd_, data_);
    }
    catch(std::exception& e_)
    {
        LOGERROR((FFWORKER_PHP, "onSessionReq failed er=<%s>", e_.what()));
    }
    return 0;
}

int FFWorkerPhp::onSessionOffline(userid_t session_id)
{
    try
    {
        Singleton<FFWorkerPhp>::instance().m_php->call_void("onSessionOffline", session_id);
    }
    catch(std::exception& e_)
    {
        LOGERROR((FFWORKER_PHP, "onSessionOffline failed er=<%s>", e_.what()));
    }
    return 0;
}
int FFWorkerPhp::FFWorkerPhp::onSessionEnter(userid_t session_id, const std::string& extra_data)
{
    try
    {
        Singleton<FFWorkerPhp>::instance().m_php->call_void("onSessionEnter", session_id, extra_data);
    }
    catch(std::exception& e_)
    {
        LOGERROR((FFWORKER_PHP, "onSessionEnter failed er=<%s>", e_.what()));
    }
    return 0;
}

std::string FFWorkerPhp::onWorkerCall(uint16_t cmd, const std::string& body)
{
    std::string ret;
    try
    {
        ret = Singleton<FFWorkerPhp>::instance().m_php->call_string("onWorkerCall", cmd, body);
    }
    catch(std::exception& e_)
    {
        ret = "!";
        ret += e_.what();
        LOGERROR((FFWORKER_PHP, "onWorkerCall failed er=<%s>", ret));
    }

    return ret;
}
//php-5.6.30]$ ./configure --enable-embed  --prefix=/home/evan/software/php5dir --with-iconv=/usr/local/libiconv
