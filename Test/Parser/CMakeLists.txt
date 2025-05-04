set(CHOCOPY_TOOLS_BINARY_DIR ${CHOCOPY_BINARY_DIR}/bin)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/Bad/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/Bad/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/Bad/lit.cfg.py
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/Stage_A/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/Stage_A/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/Stage_A/lit.cfg.py
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/Stage_B/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/Stage_B/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/Stage_B/lit.cfg.py
)

add_lit_testsuite(check-chpy-parser-Stage_A "Running the Chocopy Stage_A parser tests"
  ${CMAKE_CURRENT_BINARY_DIR}/Stage_A
)

add_lit_testsuite(check-chpy-parser-Stage_B "Running the Chocopy Stage_B parser tests"
  ${CMAKE_CURRENT_BINARY_DIR}/Stage_B
)

add_lit_testsuite(check-chpy-parser-bad "Running the Chocopy bad parser tests"
  ${CMAKE_CURRENT_BINARY_DIR}/Bad
)
