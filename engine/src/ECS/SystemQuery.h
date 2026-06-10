#ifndef PIPEFRAME_SYSTEM_QUERY_H
#define PIPEFRAME_SYSTEM_QUERY_H

#define PF_QUERY_FIELD(ComponentType, FieldName) (ComponentType, FieldName)

#define PF_DETAIL_QUERY_DECLARE(Field) PF_DETAIL_QUERY_DECLARE_ Field
#define PF_DETAIL_QUERY_DECLARE_(ComponentType, FieldName) ComponentType& FieldName;

#define PF_DETAIL_QUERY_REQUIRE(Field) PF_DETAIL_QUERY_REQUIRE_ Field
#define PF_DETAIL_QUERY_REQUIRE_(ComponentType, FieldName) system.RequireComponent<ComponentType>();

#define PF_DETAIL_QUERY_BUILD(Field) PF_DETAIL_QUERY_BUILD_ Field
#define PF_DETAIL_QUERY_BUILD_(ComponentType, FieldName) entity.GetComponent<ComponentType>(),

#define PF_DETAIL_FOR_EACH_1(Macro, Arg1) Macro(Arg1)
#define PF_DETAIL_FOR_EACH_2(Macro, Arg1, ...) Macro(Arg1) PF_DETAIL_FOR_EACH_1(Macro, __VA_ARGS__)
#define PF_DETAIL_FOR_EACH_3(Macro, Arg1, ...) Macro(Arg1) PF_DETAIL_FOR_EACH_2(Macro, __VA_ARGS__)
#define PF_DETAIL_FOR_EACH_4(Macro, Arg1, ...) Macro(Arg1) PF_DETAIL_FOR_EACH_3(Macro, __VA_ARGS__)
#define PF_DETAIL_FOR_EACH_5(Macro, Arg1, ...) Macro(Arg1) PF_DETAIL_FOR_EACH_4(Macro, __VA_ARGS__)
#define PF_DETAIL_FOR_EACH_6(Macro, Arg1, ...) Macro(Arg1) PF_DETAIL_FOR_EACH_5(Macro, __VA_ARGS__)
#define PF_DETAIL_FOR_EACH_7(Macro, Arg1, ...) Macro(Arg1) PF_DETAIL_FOR_EACH_6(Macro, __VA_ARGS__)
#define PF_DETAIL_FOR_EACH_8(Macro, Arg1, ...) Macro(Arg1) PF_DETAIL_FOR_EACH_7(Macro, __VA_ARGS__)

#define PF_DETAIL_GET_FOR_EACH_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, Name, ...) Name
#define PF_DETAIL_FOR_EACH(Macro, ...) \
    PF_DETAIL_GET_FOR_EACH_MACRO( \
        __VA_ARGS__, \
        PF_DETAIL_FOR_EACH_8, \
        PF_DETAIL_FOR_EACH_7, \
        PF_DETAIL_FOR_EACH_6, \
        PF_DETAIL_FOR_EACH_5, \
        PF_DETAIL_FOR_EACH_4, \
        PF_DETAIL_FOR_EACH_3, \
        PF_DETAIL_FOR_EACH_2, \
        PF_DETAIL_FOR_EACH_1 \
    )(Macro, __VA_ARGS__)

#define PF_SYSTEM_QUERY(...) \
    struct Query \
    { \
        PF_DETAIL_FOR_EACH(PF_DETAIL_QUERY_DECLARE, __VA_ARGS__) \
        static void Require(EntitySystem& system) \
        { \
            PF_DETAIL_FOR_EACH(PF_DETAIL_QUERY_REQUIRE, __VA_ARGS__) \
        } \
        static Query Build(Entity entity) \
        { \
            return {PF_DETAIL_FOR_EACH(PF_DETAIL_QUERY_BUILD, __VA_ARGS__)}; \
        } \
    }; \
    void RequireSystemQuery() \
    { \
        RequireQuery<Query>(); \
    } \
    Query GetSystemQuery(Entity entity) \
    { \
        return GetQuery<Query>(entity); \
    } \
    template <typename TCallback> \
    void ForEachSystemQuery(TCallback&& callback) \
    { \
        ForEach<Query>(std::forward<TCallback>(callback)); \
    }

#endif // PIPEFRAME_SYSTEM_QUERY_H
