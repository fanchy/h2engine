#ifndef _ANY_TYPE_H_
#define _ANY_TYPE_H_

#include <string>
#include "base/fftype.h"
#include "base/smart_ptr.h"

namespace ff{

class AnyTypeDataBase{
public:
    virtual ~AnyTypeDataBase(){}
    virtual void* data()  = 0 ;
    virtual const std::string& getName()  = 0 ;
};
template<typename T>
class AnyTypeDataObj: public AnyTypeDataBase{
public:
    AnyTypeDataObj(){
        m_pData = new T();
        name = TYPE_NAME(T);
    }
    AnyTypeDataObj(const T& v){
        m_pData = new T(v);
        name = TYPE_NAME(T);
    }
    ~AnyTypeDataObj(){
        delete m_pData;
        m_pData = NULL;
    }
    virtual void* data()  { return m_pData; }
    virtual const std::string& getName()  { return name; }
    T& getData()  { return *m_pData; }
public:
    T*          m_pData;
    std::string name;
};

class AnyType
{
public:
    AnyType(){
    }
    template<typename T>
    AnyType(const T& v){
        data = new AnyTypeDataObj<T>(v);
    }
    bool isType(const std::string& name){
        if (!data){
            return false;
        }
        return data->getName() == name;
    }
    template<typename T>
    bool isType(){
        return isType(TYPE_NAME(T));
    }
    const std::string& getName() {
        if (!data){
            static std::string snone;
            return snone;
        }
        return data->getName();
    }
    template<typename T>
    T& getData()
    {
        if (!isType<T>()){
            data = new AnyTypeDataObj<T>();
        }
        return ((AnyTypeDataObj<T>*)(data.get()))->getData();
    }

public:
    SharedPtr<AnyTypeDataBase>  data;
};

}
#endif
