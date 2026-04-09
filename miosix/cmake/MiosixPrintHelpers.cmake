# Utility function for printing a list
function(miosix_print_list MESSAGE_TYPE HEADER THE_LIST)
    set(STR_ITEMS "")
    foreach(LIST_ITEM ${THE_LIST})
        string(APPEND STR_ITEMS "\n   - ${LIST_ITEM}")
    endforeach()
    message(${MESSAGE_TYPE} "${HEADER}${STR_ITEMS}")
endfunction()

# Utility function for notifying the user that they have not set a variable
# correctly, if that variable needed to be set from a list
function(miosix_expected_item_in_list MESSAGE THE_LIST)
    set(STR_ITEMS "")
    foreach(LIST_ITEM ${THE_LIST})
        string(APPEND STR_ITEMS "\n   - ${LIST_ITEM}")
    endforeach()
    message(FATAL_ERROR
        " * ${MESSAGE}\n"
        " * Remember to delete cmake cache for changes in CMakeLists.txt to have effect (or use --fresh to ignore it)\n"
        " * Available options:${STR_ITEMS}")
endfunction()
