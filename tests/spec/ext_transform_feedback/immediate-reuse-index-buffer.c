/*
 * Copyright © 2011 Intel Corporation
 * Copyright © 2014 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file immediate-reuse-index-buffer.c
 *
 * Verify that if a transform feedback output buffer is immediately re-used
 * as an index buffer (changing no GL settings except for buffer bindings),
 * rendering is correct.
 *
 * The test operates by using an index buffer <-> transform feedback loop
 * that increments the vertex index (gl_VertexID). The test starts
 * with index 0 and transform feedback writes (gl_VertexID+1). Then it uses
 * the output as an input again and the index (gl_VertexID) should be 1.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;

	config.window_width = 256;
	config.window_height = 16;
	config.window_visual = PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_RGB;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

static const char *vstext =
	"#version 130\n"
	"varying vec4 out_color;\n"
	"varying int index;\n"
	"\n"
	"void main()\n"
	"{\n"
	"  int x = 8 + 16 * gl_VertexID;\n"
	"  gl_Position = vec4(x / 128.0 - 1.0, 0, 0, 1);\n"
	"  out_color = vec4(float(gl_VertexID) / 16.0, \n"
	"                   float(16 - gl_VertexID) / 16.0, \n"
	"                   float(gl_VertexID) / 16.0, 1.0);\n"
	"  index = gl_VertexID + 1;\n"
	"}\n";

static const char *fstext =
	"#version 130\n"
	"varying vec4 out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"  gl_FragColor = out_color;\n"
	"}\n";

static const char *varyings[] = { "index" };

static GLuint bufs[2];
static GLuint prog;

void
piglit_init(int argc, char **argv)
{
	piglit_require_gl_version(30);

	prog = piglit_build_simple_program_unlinked(vstext, fstext);
	glTransformFeedbackVaryings(prog, 1, varyings, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(prog);
	if (!piglit_link_check_status(prog)) {
		glDeleteProgram(prog);
		piglit_report_result(PIGLIT_FAIL);
	}

	glGenBuffers(2, bufs);
}

enum piglit_result
piglit_display(void)
{
	int i;
	bool pass = true;
	unsigned zero = 0;

	/* Setup program and initial buffer contents */
	glBindBuffer(GL_ARRAY_BUFFER, bufs[0]);
	glBufferData(GL_ARRAY_BUFFER, 4, &zero, GL_STREAM_COPY);
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, bufs[1]);
	glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 4, &zero, GL_STREAM_COPY);

	glUseProgram(prog);
	glClear(GL_COLOR_BUFFER_BIT);
	glPointSize(16);

	/* Draw 16 times, swapping transform feedback and index data
	 * so that transform feedback output is fed back to index
	 * input.
	 */
	for (i = 0; i < 16; ++i) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs[i % 2]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
				 bufs[(i + 1) % 2]);
		glBeginTransformFeedback(GL_POINTS);
		glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, NULL);
		glEndTransformFeedback();
	}

	/* Check that the proper gradient was drawn */
	for (i = 0; i < 16; ++i) {
		float expected_color[3] = {i/16.0, (16 - i)/16.0, i/16.0 };
		pass = piglit_probe_rect_rgb(16 * i, 0, 16, 16,
					     expected_color) && pass;
	}

	piglit_present_results();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}
