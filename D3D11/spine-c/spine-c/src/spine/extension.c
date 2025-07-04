/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated April 5, 2025. Replaces all prior versions.
 *
 * Copyright (c) 2013-2025, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software
 * or otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THE SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <spine/extension.h>
#include <stdio.h>

float _spInternalRandom(void) {
	return rand() / (float) RAND_MAX;
}

static void *(*mallocFunc)(size_t size) = malloc;

static void *(*reallocFunc)(void *ptr, size_t size) = realloc;

static void *(*debugMallocFunc)(size_t size, const char *file, int line) = NULL;

static void (*freeFunc)(void *ptr) = free;

static float (*randomFunc)(void) = _spInternalRandom;

void *_spMalloc(size_t size, const char *file, int line) {
	if (debugMallocFunc)
		return debugMallocFunc(size, file, line);

	return mallocFunc(size);
}

void *_spCalloc(size_t num, size_t size, const char *file, int line) {
	void *ptr = _spMalloc(num * size, file, line);
	if (ptr) memset(ptr, 0, num * size);
	return ptr;
}

void *_spRealloc(void *ptr, size_t size) {
	return reallocFunc(ptr, size);
}

void _spFree(void *ptr) {
	freeFunc(ptr);
}

float _spRandom(void) {
	return randomFunc();
}

void _spSetDebugMalloc(void *(*malloc)(size_t size, const char *file, int line)) {
	debugMallocFunc = malloc;
}

void _spSetMalloc(void *(*malloc)(size_t size)) {
	mallocFunc = malloc;
}

void _spSetRealloc(void *(*realloc)(void *ptr, size_t size)) {
	reallocFunc = realloc;
}

void _spSetFree(void (*free)(void *ptr)) {
	freeFunc = free;
}

void _spSetRandom(float (*random)(void)) {
	randomFunc = random;
}

char *_spReadFile(const char *path, int *length) {
	char *data;
	size_t result;
	FILE *file = fopen(path, "rb");
	if (!file) return 0;

	fseek(file, 0, SEEK_END);
	*length = (int) ftell(file);
	fseek(file, 0, SEEK_SET);

	data = MALLOC(char, *length);
	result = fread(data, 1, *length, file);
	UNUSED(result);
	fclose(file);

	return data;
}

float _spMath_random(float min, float max) {
	return min + (max - min) * _spRandom();
}

float _spMath_randomTriangular(float min, float max) {
	return _spMath_randomTriangularWith(min, max, (min + max) * 0.5f);
}

float _spMath_randomTriangularWith(float min, float max, float mode) {
	float u = _spRandom();
	float d = max - min;
	if (u <= (mode - min) / d) return min + SQRT(u * d * (mode - min));
	return max - SQRT((1 - u) * d * (max - mode));
}

float _spMath_interpolate(float (*apply)(float a), float start, float end, float a) {
	return start + (end - start) * apply(a);
}

float _spMath_pow2_apply(float a) {
	if (a <= 0.5) return POW(a * 2, 2) / 2;
	return POW((a - 1) * 2, 2) / -2 + 1;
}

float _spMath_pow2out_apply(float a) {
	return POW(a - 1, 2) * -1 + 1;
}
