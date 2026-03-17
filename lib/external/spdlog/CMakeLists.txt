set(SPDLOG_INSTALL ON CACHE BOOL "")

# spdlog is naughty and has an explicit option for building static or
# shared, rather than relying on BUILD_SHARED_LIBS; this can create
# mismatched shared-and-static combinations that lead to relocatable
# code errors in the build.
if(BUILD_SHARED_LIBS)
    set(SPDLOG_BUILD_SHARED ON CACHE BOOL "")
endif()

opendaq_dependency(
    NAME                spdlog
    REQUIRED_VERSION    1.16.0
    GIT_REPOSITORY      https://github.com/gabime/spdlog.git
    GIT_REF             v1.16.0
    EXPECT_TARGET       spdlog::spdlog
)
