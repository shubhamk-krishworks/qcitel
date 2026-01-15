# ============================================================
# CMakeLists.txt
# Project Component Registration for ESP-IDF
# ------------------------------------------------------------
# This file automatically collects all .c and .cpp source files
# (including from subdirectories like eco_queue, models, etc.)
# and registers them as part of this ESP-IDF component.
#
# Author: <Your Name>
# Date: <Optional>
# ============================================================

# ------------------------------------------------------------
# Collect all C and C++ source files recursively
# ------------------------------------------------------------
file(GLOB_RECURSE SRC_FILES
    "*.c"
    "*.cpp"
)

# ------------------------------------------------------------
# Register this component with ESP-IDF build system
# ------------------------------------------------------------
idf_component_register(
    SRCS ${SRC_FILES}                # All collected source files
    INCLUDE_DIRS                     # Public include paths
        "."                          # Current directory
        "hal"
        "oal"
        "app"
        "examples"
)

# ------------------------------------------------------------
# (Optional) Print debug info for verification
# Uncomment below lines if you want to see which files were found.
# message(STATUS "Component sources: ${SRC_FILES}")
# ============================================================