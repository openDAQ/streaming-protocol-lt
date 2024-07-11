file(COPY_FILE  ${CMAKE_CURRENT_SOURCE_DIR}/post-xxx-sample.txt ${CMAKE_SOURCE_DIR}/.git/hooks/post-commit)
file(COPY_FILE  ${CMAKE_CURRENT_SOURCE_DIR}/post-xxx-sample.txt ${CMAKE_SOURCE_DIR}/.git/hooks/post-checkout)
file(CHMOD ${CMAKE_SOURCE_DIR}/.git/hooks/post-commit ${CMAKE_SOURCE_DIR}/.git/hooks/post-checkout FILE_PERMISSIONS  OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE  )

set(PDF_OPTS
  -s -f markdown -t latex --template openDAQ.template
)

add_custom_target(openDAQ-streaming.pdf ALL
  pandoc ${PDF_OPTS}  -o ${CMAKE_CURRENT_BINARY_DIR}/openDAQ-streaming.pdf openDAQ-streaming.md
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}

)

add_custom_target(openDAQ-streaming-optional-features.pdf ALL
  pandoc ${PDF_OPTS}  -o ${CMAKE_CURRENT_BINARY_DIR}/openDAQ-streaming-optional-features.pdf openDAQ-streaming-optional-features.md
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(openDAQ-streaming-workflow.pdf ALL
  pandoc ${PDF_OPTS}  -o ${CMAKE_CURRENT_BINARY_DIR}/openDAQ-streaming-workflow.pdf openDAQ-streaming-workflow.md
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(openDAQ-signal-definition-examples.pdf ALL
  pandoc ${PDF_OPTS}  -o ${CMAKE_CURRENT_BINARY_DIR}/openDAQ-signal-definition-examples.pdf openDAQ-signal-definition-examples.md
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#### HTML

set(HTML_OPTS -s --css=openDAQ.css --toc --toc-depth=3 --mathjax -N -f markdown)
add_custom_target(openDAQ-streaming.html ALL
  pandoc ${HTML_OPTS} -o ${CMAKE_CURRENT_BINARY_DIR}/openDAQ-streaming.html openDAQ-streaming.md
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(openDAQ-streaming-optional-features.html ALL
  pandoc ${HTML_OPTS}   -o ${CMAKE_CURRENT_BINARY_DIR}/openDAQ-streaming-optional-features.html openDAQ-streaming-optional-features.md
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(openDAQ-streaming-workflow.html ALL
  pandoc ${HTML_OPTS}  -o ${CMAKE_CURRENT_BINARY_DIR}/openDAQ-streaming-workflow.html openDAQ-streaming-workflow.md
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(openDAQ-signal-definition-examples.html ALL
  pandoc ${HTML_OPTS}  -o ${CMAKE_CURRENT_BINARY_DIR}/openDAQ-signal-definition-examples.html openDAQ-signal-definition-examples.md
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
