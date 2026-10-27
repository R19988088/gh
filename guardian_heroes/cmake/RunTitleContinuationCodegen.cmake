find_program(_gh_title_python NAMES python3 REQUIRED)
execute_process(COMMAND "${_gh_title_python}" "${GH_TITLE_CODEGEN}"
    "${GH_TITLE_SOURCE}" "${GH_TITLE_HOOKS}" "${GH_TITLE_OUTPUT}"
    COMMAND_ERROR_IS_FATAL ANY)
