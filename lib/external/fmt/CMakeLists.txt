set(FMT_INSTALL ON CACHE BOOL "" FORCE)

opendaq_dependency(
    NAME                fmt
    REQUIRED_VERSION    12.1.0
    GIT_REPOSITORY      https://github.com/fmtlib/fmt.git
    GIT_REF             12.1.0
    EXPECT_TARGET       fmt::fmt
)
