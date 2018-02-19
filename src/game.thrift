namespace cpp GameMsg

struct Item{
    1:i64 id                  = 0
    2:i32 cfgid               = 0
    3:i32 pos                 = 0
}
struct PkgInfoRet{//!包裹数据
    1:list<Item>            allItem
}
