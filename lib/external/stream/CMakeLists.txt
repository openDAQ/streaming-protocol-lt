opendaq_dependency(
    NAME                stream
    REQUIRED_VERSION    1.0.4
    GIT_REPOSITORY      ${OPENDAQ_REPO_PREFIX}/libstream.git
    GIT_REF             jira/TBBAS-3109-apply-opendaq-cmake-utils
    GIT_SHALLOW         OFF
    EXPECT_TARGET       daq::stream
)
