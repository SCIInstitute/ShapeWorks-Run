
SET(RUN_NAME ShapeWorksRun5.0)

ADD_EXECUTABLE(${RUN_NAME} ShapeWorksRun.cxx ShapeWorksRunApp.h ShapeWorksRunApp.txx)

TARGET_LINK_LIBRARIES(${RUN_NAME} ITKParticleSystem Utilities ${ITK_LIBRARIES} ${VTK_LIBRARIES} tinyxml fim_v4 vgl vgl_algo)

INSTALL(TARGETS ${RUN_NAME}  RUNTIME DESTINATION .)
