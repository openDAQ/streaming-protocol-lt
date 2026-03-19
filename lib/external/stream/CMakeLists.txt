opendaq_dependency(
    NAME                stream
    REQUIRED_VERSION    1.0.4
    GIT_REPOSITORY      ${OPENDAQ_REPO_PREFIX}/libstream.git
    GIT_REF             v1.0.4-dev
    GIT_SHALLOW         OFF
    EXPECT_TARGET       daq::stream
)
