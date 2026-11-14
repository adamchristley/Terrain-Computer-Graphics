#include "libkuhl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define GRID_WIDTH 640
#define GRID_HEIGHT 320
#define CLOUD_HEIGHT 0.31f

static GLuint groundProgram, cloudProgram;
static GLuint heightTexture, colorTexture, cloudTexture;
static kuhl_geometry ground, clouds;
static int showNormals = 0;
static int automaticCamera = 1;

void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if(kuhl_keyboard_handler(window, key, scancode, action, mods))
        return;
    if(action != GLFW_PRESS)
        return;

    if(key == GLFW_KEY_N) {
        showNormals = !showNormals;
        printf("Normals: %s\n", showNormals ? "on" : "off");
    }
    if(key == GLFW_KEY_SPACE) {
        automaticCamera = !automaticCamera;
        printf("Camera: %s\n", automaticCamera ? "automatic" : "manual");
    }
}

/* Make a 1 by 1 grid. The shader will use the heightmap for its Y values. */
void makeGround(void)
{
    int width = GRID_WIDTH + 1;
    int numVertices = width * (GRID_HEIGHT + 1);
    int numIndices = GRID_WIDTH * GRID_HEIGHT * 6;
    GLfloat *positions = malloc(numVertices * 3 * sizeof(GLfloat));
    GLuint *indices = malloc(numIndices * sizeof(GLuint));

    if(positions == NULL || indices == NULL) {
        fprintf(stderr, "Not enough memory for terrain\n");
        exit(EXIT_FAILURE);
    }

    int n = 0;
    for(int z = 0; z <= GRID_HEIGHT; z++) {
        for(int x = 0; x <= GRID_WIDTH; x++) {
            positions[n++] = x / (float)GRID_WIDTH;
            positions[n++] = 0.0f;
            positions[n++] = z / (float)GRID_HEIGHT;
        }
    }

    n = 0;
    for(int z = 0; z < GRID_HEIGHT; z++) {
        for(int x = 0; x < GRID_WIDTH; x++) {
            int a = z * width + x;
            int b = a + 1;
            int c = a + width;
            int d = c + 1;

            indices[n++] = a;
            indices[n++] = c;
            indices[n++] = b;
            indices[n++] = b;
            indices[n++] = c;
            indices[n++] = d;
        }
    }

    kuhl_geometry_new(&ground, groundProgram, numVertices, GL_TRIANGLES);
    kuhl_geometry_attrib(&ground, positions, 3, "in_Position", KG_WARN);
    kuhl_geometry_indices(&ground, indices, numIndices);
    kuhl_geometry_texture(&ground, heightTexture, "heightTex", KG_WARN);
    kuhl_geometry_texture(&ground, colorTexture, "colorTex", KG_WARN);

    free(positions);
    free(indices);
}

/* Only two triangles are needed for the transparent clouds. */
void makeClouds(void)
{
    GLfloat positions[] = {
        0, CLOUD_HEIGHT, 0,
        0, CLOUD_HEIGHT, 1,
        1, CLOUD_HEIGHT, 0,
        1, CLOUD_HEIGHT, 0,
        0, CLOUD_HEIGHT, 1,
        1, CLOUD_HEIGHT, 1
    };

    kuhl_geometry_new(&clouds, cloudProgram, 6, GL_TRIANGLES);
    kuhl_geometry_attrib(&clouds, positions, 3, "in_Position", KG_WARN);
    kuhl_geometry_texture(&clouds, cloudTexture, "cloudTex", KG_WARN);
}

void display(void)
{
    viewmat_begin_frame();
    for(int viewportID = 0; viewportID < viewmat_num_viewports(); viewportID++) {
        viewmat_begin_eye(viewportID);

        int viewport[4];
        viewmat_get_viewport(viewport, viewportID);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

        glEnable(GL_SCISSOR_TEST);
        glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
        glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_DEPTH_TEST);

        float view[16], projection[16], modelview[16];
        viewmat_get(view, projection, viewportID);

        if(automaticCamera) {
            /* Circle around the map, changing height above and below the clouds. */
            float t = (float)glfwGetTime() * 0.12f;
            float eye[3] = {
                0.78f * cosf(t),
                0.34f + 0.15f * sinf(t + 0.6f),
                0.40f * sinf(t)
            };
            float target[3] = {
                0.15f * cosf(t + 1.2f), 0.05f, 0.0f
            };
            float up[3] = {0, 1, 0};
            mat4f_lookatVec_new(view, eye, target, up);
            mat4f_perspective_new(projection, 60.0f,
                                  viewport[2] / (float)viewport[3],
                                  0.01f, 12.0f);
        }

        /* Turn the 1x1 surface into a centered, 2x1 world map. */
        float move[16], stretch[16], model[16];
        mat4f_translate_new(move, -1.0f, 0.0f, -0.5f);
        mat4f_scale_new(stretch, 2.0f, 1.0f, 1.0f);
        mat4f_mult_mat4f_many(model, move, stretch, NULL);
        mat4f_mult_mat4f_many(modelview, view, model, NULL);

        glUseProgram(groundProgram);
        glUniformMatrix4fv(kuhl_get_uniform("Projection"),
                           1, GL_FALSE, projection);
        glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
                           1, GL_FALSE, modelview);
        glUniform1i(kuhl_get_uniform("showNormals"), showNormals);
        kuhl_geometry_draw(&ground);

        /* Draw clouds last so their alpha blends with the ground. */
        if(!showNormals) {
            glEnable(GL_BLEND);
            glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                GL_ONE, GL_ZERO);
            glDepthMask(GL_FALSE);

            glUseProgram(cloudProgram);
            glUniformMatrix4fv(kuhl_get_uniform("Projection"),
                               1, GL_FALSE, projection);
            glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
                               1, GL_FALSE, modelview);
            kuhl_geometry_draw(&clouds);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        glUseProgram(0);
        viewmat_end_eye(viewportID);
    }
    viewmat_end_frame();
    kuhl_errorcheck();
}

int main(int argc, char **argv)
{
    kuhl_ogl_init(&argc, argv, -1, -1, 32, 4);
    glfwSetKeyCallback(kuhl_get_window(), keyboard);

    groundProgram = kuhl_create_program("terrain-ground.vert",
                                         "terrain-ground.frag");
    cloudProgram = kuhl_create_program("terrain-cloud.vert",
                                        "terrain-cloud.frag");

    kuhl_read_texture_file("terrain-height.jpg", &heightTexture);
    kuhl_read_texture_file("terrain-color.jpg", &colorTexture);
    kuhl_read_texture_file("terrain-clouds.jpg", &cloudTexture);

    if(heightTexture == 0 || colorTexture == 0 || cloudTexture == 0) {
        fprintf(stderr, "Could not load terrain images\n");
        return EXIT_FAILURE;
    }

    makeGround();
    makeClouds();

    dgr_init();
    float cameraPos[3] = {0.0f, 0.65f, 1.35f};
    float cameraLook[3] = {0.0f, 0.05f, 0.0f};
    float cameraUp[3] = {0, 1, 0};
    viewmat_init(cameraPos, cameraLook, cameraUp);

    printf("Space: camera mode | N: normals | Shift+W: wireframe\n");
    while(!glfwWindowShouldClose(kuhl_get_window())) {
        display();
        glfwPollEvents();
    }
    return EXIT_SUCCESS;
}
