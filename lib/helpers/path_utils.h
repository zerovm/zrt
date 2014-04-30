/*
 *
 * Copyright (c) 2014, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define DEFAULT_TEMP_CURSOR_VALUE -100
#define INIT_TEMP_CURSOR(temp_cursor_p) *(temp_cursor_p) = DEFAULT_TEMP_CURSOR_VALUE

/*example path "/1/22/" components = { "/", "1", "22", NULL }*/
const char *path_component_forward(int *temp_cursor, const char *path, int *result_len);

/*example path "/1/22/" components = { "22", "1", "/", NULL }*/
const char *path_component_backward(int *temp_cursor, const char *path, int *result_len);


/*example path "/1/22/" components = { "/", "/1", "/1/22", NULL }*/
const char *path_subpath_forward(int *temp_cursor, const char *path, int *result_len);


/*example path "/1/22/" components = { "/1/22", "/1", "/", NULL }*/
const char *path_subpath_backward(int *temp_cursor, const char *path, int *result_len);

int test_path_utils();

/*if path not started with slash or have dots .. then assume the path
  is related;
  @return 1 relative, 0 not relative*/
int is_relative_path(const char *path);
