if(NOT EXISTS "${destination}")
  file(COPY_FILE "${source}" "${destination}")
endif()
