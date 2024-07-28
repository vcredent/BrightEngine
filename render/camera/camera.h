/* ======================================================================== */
/* camera.h                                                                 */
/* ======================================================================== */
/*                        This file is part of:                             */
/*                           COPILOT ENGINE                                 */
/* ======================================================================== */
/*                                                                          */
/* Copyright (C) 2022 Vcredent All rights reserved.                         */
/*                                                                          */
/* Licensed under the Apache License, Version 2.0 (the "License");          */
/* you may not use this file except in compliance with the License.         */
/*                                                                          */
/* You may obtain a copy of the License at                                  */
/*     http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                          */
/* Unless required by applicable law or agreed to in writing, software      */
/* distributed under the License is distributed on an "AS IS" BASIS,        */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied  */
/* See the License for the specific language governing permissions and      */
/* limitations under the License.                                           */
/*                                                                          */
/* ======================================================================== */
#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <copilot/math.h>
#include <copilot/typedefs.h>
#include <algorithm>

class Camera {
public:
    V_FORCEINLINE inline Vector3 get_position() { return position; }
    V_FORCEINLINE inline Vector3 get_world_right() { return world_right; }
    V_FORCEINLINE inline Vector3 get_world_up() { return world_up; }
    V_FORCEINLINE inline Vector3 get_target() { return target; }
    V_FORCEINLINE inline float get_pitch() { return pitch; }
    V_FORCEINLINE inline float get_yaw() { return yaw; }
    V_FORCEINLINE inline float get_roll() { return roll; }
    V_FORCEINLINE inline float get_aspect_ratio() { return aspect_ratio; }
    V_FORCEINLINE inline float get_speed() { return speed; }
    V_FORCEINLINE inline float get_sensitivity() { return sensitivity; }
    V_FORCEINLINE inline Matrix4& get_view_matrix() { return view_matrix; }
    V_FORCEINLINE inline Matrix4& get_projection_matrix() { return projection_matrix; }

    V_FORCEINLINE inline void set_position(Vector3 v_position) { position = v_position; }
    V_FORCEINLINE inline void set_world_right(Vector3 v_world_right) { world_right = v_world_right; }
    V_FORCEINLINE inline void set_world_up(Vector3 v_world_up) { world_up = v_world_up; }
    V_FORCEINLINE inline void set_target(Vector3 v_target) { target = v_target; }
    V_FORCEINLINE inline void set_pitch(float v_pitch) { pitch = glm::clamp(v_pitch, -89.0f, 89.0f); }
    V_FORCEINLINE inline void set_yaw(float v_yaw) { yaw = v_yaw; }
    V_FORCEINLINE inline void set_roll(float v_roll) { roll = v_roll; }
    V_FORCEINLINE inline void set_aspect_ratio(float v_aspect_ratio) { aspect_ratio = v_aspect_ratio; }
    V_FORCEINLINE inline void set_speed(float v_speed) { speed = v_speed; }
    V_FORCEINLINE inline void set_sensitivity(float v_sensitivity) { sensitivity = v_sensitivity; }
    V_FORCEINLINE inline void get_view_matrix(Matrix4 &v_view_matrix) { view_matrix = v_view_matrix; }
    V_FORCEINLINE inline void get_projection_matrix(Matrix4 &v_projection_matrix) { projection_matrix = v_projection_matrix; }

    virtual void update() = 0;

protected:
    Vector3 position = Vector3(0.0f, 0.0f, 3.0f);
    Vector3 world_right = Vector3(1.0f, 0.0f, 0.0f);
    Vector3 world_up = Vector3(0.0f, 1.0f, 0.0f);
    Vector3 target = Vector3(0.0f, 0.0f, 0.0f);

    float pitch = 0.0f;
    float yaw = -90.0f;
    float roll = 0.0f;
    float aspect_ratio = 1.0f;

    float speed = 0.5f;
    float sensitivity = 0.5f;

    Matrix4 view_matrix = Matrix4(1.0f);
    Matrix4 projection_matrix = Matrix4(1.0f);
};

#endif /* _CAMERA_H_ */