/**
 * @file ttfparser.h
 *
 * A C API for the Rust's ttf-parser library.
 */

#ifndef TTFP_H
#define TTFP_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief A glyph image format.
 */
typedef enum ttfp_raster_image_format {
    /**
     * @brief A PNG.
     */
    TTFP_RASTER_IMAGE_FORMAT_PNG = 0,
    /**
     * @brief A monochrome bitmap.
     *
     * The most significant bit of the first byte corresponds to the top-left pixel, proceeding
     * through succeeding bits moving left to right. The data for each row is padded to a byte
     * boundary, so the next row begins with the most significant bit of a new byte. 1 corresponds
     * to black, and 0 to white.
     */
    TTFP_RASTER_IMAGE_FORMAT_BITMAP_MONO = 1,
    /**
     * @brief A packed monochrome bitmap.
     *
     * The most significant bit of the first byte corresponds to the top-left pixel, proceeding
     * through succeeding bits moving left to right. Data is tightly packed with no padding. 1
     * corresponds to black, and 0 to white.
     */
    TTFP_RASTER_IMAGE_FORMAT_BITMAP_MONO_PACKED = 2,
    /**
     * @brief A grayscale bitmap with 2 bits per pixel.
     *
     * The most significant bits of the first byte corresponds to the top-left pixel, proceeding
     * through succeeding bits moving left to right. The data for each row is padded to a byte
     * boundary, so the next row begins with the most significant bit of a new byte.
     */
    TTFP_RASTER_IMAGE_FORMAT_BITMAP_GRAY_2 = 3,
    /**
     * @brief A packed grayscale bitmap with 2 bits per pixel.
     *
     * The most significant bits of the first byte corresponds to the top-left pixel, proceeding
     * through succeeding bits moving left to right. Data is tightly packed with no padding.
     */
    TTFP_RASTER_IMAGE_FORMAT_BITMAP_GRAY_2_PACKED = 4,
    /**
     * @brief A grayscale bitmap with 4 bits per pixel.
     *
     * The most significant bits of the first byte corresponds to the top-left pixel, proceeding
     * through succeeding bits moving left to right. The data for each row is padded to a byte
     * boundary, so the next row begins with the most significant bit of a new byte.
     */
    TTFP_RASTER_IMAGE_FORMAT_BITMAP_GRAY_4 = 5,
    /**
     * @brief A packed grayscale bitmap with 4 bits per pixel.
     *
     * The most significant bits of the first byte corresponds to the top-left pixel, proceeding
     * through succeeding bits moving left to right. Data is tightly packed with no padding.
     */
    TTFP_RASTER_IMAGE_FORMAT_BITMAP_GRAY_4_PACKED = 6,
    /**
     * @brief A grayscale bitmap with 8 bits per pixel.
     *
     * The first byte corresponds to the top-left pixel, proceeding through succeeding bytes
     * moving left to right.
     */
    TTFP_RASTER_IMAGE_FORMAT_BITMAP_GRAY_8 = 7,
    /**
     * @brief A color bitmap with 32 bits per pixel.
     *
     * The first group of four bytes corresponds to the top-left pixel, proceeding through
     * succeeding pixels moving left to right. Each byte corresponds to a color channel and the
     * channels within a pixel are in blue, green, red, alpha order. Color values are
     * pre-multiplied by the alpha. For example, the color "full-green with half translucency"
     * is encoded as `\x00\x80\x00\x80`, and not `\x00\xFF\x00\x80`.
     */
    TTFP_RASTER_IMAGE_FORMAT_BITMAP_PREMUL_BGRA_32 = 8,
} ttfp_raster_image_format;

/**
 * @brief An opaque pointer to the font face structure.
 */
typedef struct ttfp_face ttfp_face;

/**
 * @brief A name record.
 *
 * https://docs.microsoft.com/en-us/typography/opentype/spec/name#name-records
 */
typedef struct ttfp_name_record {
    uint16_t platform_id;
    uint16_t encoding_id;
    uint16_t language_id;
    uint16_t name_id;
    uint16_t name_size;
} ttfp_name_record;

/**
 * A line metrics.
 *
 * Used for underline and strikeout.
 */
typedef struct ttfp_line_metrics {
    /**
     * Line position.
     */
    int16_t position;
    /**
     * Line thickness.
     */
    int16_t thickness;
} ttfp_line_metrics;

/**
 * A script metrics used by subscript and superscript.
 */
typedef struct ttfp_script_metrics {
    /**
     * Horizontal face size.
     */
    int16_t x_size;
    /**
     * Vertical face size.
     */
    int16_t y_size;
    /**
     * X offset.
     */
    int16_t x_offset;
    /**
     * Y offset.
     */
    int16_t y_offset;
} ttfp_script_metrics;

/**
 * A type-safe wrapper for glyph ID.
 */
typedef uint16_t uint16_t;

/**
 * @brief An outline building interface.
 */
typedef struct ttfp_outline_builder {
    void (*move_to)(float x, float y, void *data);
    void (*line_to)(float x, float y, void *data);
    void (*quad_to)(float x1, float y1, float x, float y, void *data);
    void (*curve_to)(float x1, float y1, float x2, float y2, float x, float y, void *data);
    void (*close_path)(void *data);
} ttfp_outline_builder;

/**
 * A rectangle.
 *
 * Doesn't guarantee that `x_min` <= `x_max` and/or `y_min` <= `y_max`.
 */
typedef struct ttfp_rect {
    int16_t x_min;
    int16_t y_min;
    int16_t x_max;
    int16_t y_max;
} ttfp_rect;

/**
 * @brief A glyph image.
 *
 * An image offset and size isn't defined in all tables, so `x`, `y`, `width` and `height`
 * can be set to 0.
 */
typedef struct ttfp_glyph_raster_image {
    /**
     * Horizontal offset.
     */
    int16_t x;
    /**
     * Vertical offset.
     */
    int16_t y;
    /**
     * Image width.
     *
     * It doesn't guarantee that this value is the same as set in the `data`.
     */
    uint16_t width;
    /**
     * Image height.
     *
     * It doesn't guarantee that this value is the same as set in the `data`.
     */
    uint16_t height;
    /**
     * A pixels per em of the selected strike.
     */
    uint16_t pixels_per_em;
    /**
     * An image format.
     */
    enum ttfp_raster_image_format format;
    /**
     * A raw image data as is. It's up to the caller to decode PNG, JPEG, etc.
     */
    const char *data;
    /**
     * A raw image data size.
     */
    uint32_t len;
} ttfp_glyph_raster_image;

/**
 * A 4-byte tag.
 */
typedef uint32_t ttfp_tag;

#if defined(TTFP_VARIABLE_FONTS)
/**
 * A [variation axis](https://docs.microsoft.com/en-us/typography/opentype/spec/fvar#variationaxisrecord).
 */
typedef struct ttfp_variation_axis {
    ttfp_tag tag;
    float min_value;
    float def_value;
    float max_value;
    /**
     * An axis name in the `name` table.
     */
    uint16_t name_id;
    bool hidden;
} ttfp_variation_axis;
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

uint32_t return_five(void);

/**
 * @brief Returns the number of fonts stored in a TrueType font collection.
 *
 * @param data The font data.
 * @param len The size of the font data.
 * @return Number of fonts or -1 when provided data is not a TrueType font collection
 *         or when number of fonts is larger than INT_MAX.
 */
int32_t ttfp_fonts_in_collection(const char *data, uintptr_t len);

/**
 * @brief Creates a new font face parser.
 *
 * Since #ttfp_face is an opaque pointer, a caller should allocate it manually
 * using #ttfp_face_size_of.
 * Deallocation is also handled by a caller.
 * #ttfp_face doesn't use heap internally, so we can simply `free()` it without
 * a dedicated `ttfp_face_deinit` function.
 *
 * @param data A font binary data. Must outlive the #ttfp_face.
 * @param len Size of the font data.
 * @param index The font face index in a collection (typically *.ttc). 0 should be used for basic fonts.
 * @param face A pointer to a #ttfp_face object.
 * @return `true` on success.
 */
bool ttfp_face_init(const char *data,
                    uintptr_t len,
                    uint32_t index,
                    void *face);

/**
 * @brief Returns the size of `ttfp_face`.
 */
uintptr_t ttfp_face_size_of(void);

/**
 * @brief Returns the number of name records in the face.
 */
uint16_t ttfp_get_name_records_count(const struct ttfp_face *face);

/**
 * @brief Returns a name record.
 *
 * @param Record's index. The total amount can be obtained via #ttfp_get_name_records_count.
 * @return `false` when `index` is out of range or `platform_id` is invalid.
 */
bool ttfp_get_name_record(const struct ttfp_face *face,
                          uint16_t index,
                          struct ttfp_name_record *record);

/**
 * @brief Returns a name record's string.
 *
 * @param index Record's index.
 * @param name A string buffer that will be filled with the record's name.
 *             Remember that a name will use encoding specified in `ttfp_name_record.encoding_id`
 *             Because of that, the name will not be null-terminated.
 * @param len The size of a string buffer. Must be equal to `ttfp_name_record.name_size`.
 * @return `false` when `index` is out of range or string buffer is not equal
 *         `ttfp_name_record.name_size`.
 */
bool ttfp_get_name_record_string(const struct ttfp_face *face,
                                 uint16_t index,
                                 char *name,
                                 uintptr_t len);

/**
 * @brief Checks that face is marked as *Regular*.
 *
 * @return `false` when OS/2 table is not present.
 */
bool ttfp_is_regular(const struct ttfp_face *face);

/**
 * @brief Checks that face is marked as *Italic*.
 *
 * @return `false` when OS/2 table is not present.
 */
bool ttfp_is_italic(const struct ttfp_face *face);

/**
 * @brief Checks that face is marked as *Bold*.
 *
 * @return `false` when OS/2 table is not present.
 */
bool ttfp_is_bold(const struct ttfp_face *face);

/**
 * @brief Checks that face is marked as *Oblique*.
 *
 * @return `false` when OS/2 table is not present.
 */
bool ttfp_is_oblique(const struct ttfp_face *face);

/**
 * @brief Checks that face is marked as *Monospaced*.
 *
 * @return `false` when `post` table is not present.
 */
bool ttfp_is_monospaced(const struct ttfp_face *face);

/**
 * @brief Checks that face is variable.
 *
 * Simply checks the presence of a `fvar` table.
 */
bool ttfp_is_variable(const struct ttfp_face *face);

/**
 * @brief Returns face's weight.
 *
 * @return Face's weight or `400` when OS/2 table is not present.
 */
uint16_t ttfp_get_weight(const struct ttfp_face *face);

/**
 * @brief Returns face's width.
 *
 * @return Face's width in a 1..9 range or `5` when OS/2 table is not present
 *         or when value is invalid.
 */
uint16_t ttfp_get_width(const struct ttfp_face *face);

/**
 * @brief Returns face's italic angle.
 *
 * @return Face's italic angle or `0.0` when `post` table is not present.
 */
float ttfp_get_italic_angle(const struct ttfp_face *face);

/**
 * @brief Returns a horizontal face ascender.
 *
 * This function is affected by variation axes.
 */
int16_t ttfp_get_ascender(const struct ttfp_face *face);

/**
 * @brief Returns a horizontal face descender.
 *
 * This function is affected by variation axes.
 */
int16_t ttfp_get_descender(const struct ttfp_face *face);

/**
 * @brief Returns a horizontal face height.
 *
 * This function is affected by variation axes.
 */
int16_t ttfp_get_height(const struct ttfp_face *face);

/**
 * @brief Returns a horizontal face line gap.
 *
 * This function is affected by variation axes.
 */
int16_t ttfp_get_line_gap(const struct ttfp_face *face);

/**
 * @brief Returns a horizontal typographic face ascender.
 *
 * Prefer `ttfp_get_ascender` unless you explicitly want this. This is a more
 * low-level alternative.
 *
 * This function is affected by variation axes.
 *
 * @return `0` when OS/2 table is not present.
 */
int16_t ttfp_get_typographic_ascender(const struct ttfp_face *face);

/**
 * @brief Returns a horizontal typographic face descender.
 *
 * Prefer `ttfp_get_descender` unless you explicitly want this. This is a more
 * low-level alternative.
 *
 * This function is affected by variation axes.
 *
 * @return `0` when OS/2 table is not present.
 */
int16_t ttfp_get_typographic_descender(const struct ttfp_face *face);

/**
 * @brief Returns a horizontal typographic face line gap.
 *
 * Prefer `ttfp_get_line_gap` unless you explicitly want this. This is a more
 * low-level alternative.
 *
 * This function is affected by variation axes.
 *
 * @return `0` when OS/2 table is not present.
 */
int16_t ttfp_get_typographic_line_gap(const struct ttfp_face *face);

/**
 * @brief Returns a vertical face ascender.
 *
 * This function is affected by variation axes.
 *
 * @return `0` when `vhea` table is not present.
 */
int16_t ttfp_get_vertical_ascender(const struct ttfp_face *face);

/**
 * @brief Returns a vertical face descender.
 *
 * This function is affected by variation axes.
 *
 * @return `0` when `vhea` table is not present.
 */
int16_t ttfp_get_vertical_descender(const struct ttfp_face *face);

/**
 * @brief Returns a vertical face height.
 *
 * This function is affected by variation axes.
 *
 * @return `0` when `vhea` table is not present.
 */
int16_t ttfp_get_vertical_height(const struct ttfp_face *face);

/**
 * @brief Returns a vertical face line gap.
 *
 * This function is affected by variation axes.
 *
 * @return `0` when `vhea` table is not present.
 */
int16_t ttfp_get_vertical_line_gap(const struct ttfp_face *face);

/**
 * @brief Returns face's units per EM.
 *
 * @return Units in a 16..16384 range or `0` otherwise.
 */
uint16_t ttfp_get_units_per_em(const struct ttfp_face *face);

/**
 * @brief Returns face's x height.
 *
 * This function is affected by variation axes.
 *
 * @return x height or 0 when OS/2 table is not present or when its version is < 2.
 */
int16_t ttfp_get_x_height(const struct ttfp_face *face);

/**
 * @brief Returns face's capital height.
 *
 * This function is affected by variation axes.
 *
 * @return capital height or 0 when OS/2 table is not present or when its version is < 2.
 */
int16_t ttfp_get_capital_height(const struct ttfp_face *face);

/**
 * @brief Returns face's underline metrics.
 *
 * This function is affected by variation axes.
 *
 * @return `false` when `post` table is not present.
 */
bool ttfp_get_underline_metrics(const struct ttfp_face *face, struct ttfp_line_metrics *metrics);

/**
 * @brief Returns face's strikeout metrics.
 *
 * This function is affected by variation axes.
 *
 * @return `false` when OS/2 table is not present.
 */
bool ttfp_get_strikeout_metrics(const struct ttfp_face *face, struct ttfp_line_metrics *metrics);

/**
 * @brief Returns font's subscript metrics.
 *
 * This function is affected by variation axes.
 *
 * @return `false` when OS/2 table is not present.
 */
bool ttfp_get_subscript_metrics(const struct ttfp_face *face, struct ttfp_script_metrics *metrics);

/**
 * @brief Returns face's superscript metrics.
 *
 * This function is affected by variation axes.
 *
 * @return `false` when OS/2 table is not present.
 */
bool ttfp_get_superscript_metrics(const struct ttfp_face *face,
                                  struct ttfp_script_metrics *metrics);

/**
 * @brief Returns a total number of glyphs in the face.
 *
 * @return The number of glyphs which is never zero.
 */
uint16_t ttfp_get_number_of_glyphs(const struct ttfp_face *face);

/**
 * @brief Resolves a Glyph ID for a code point.
 *
 * All subtable formats except Mixed Coverage (8) are supported.
 *
 * @param codepoint A valid Unicode codepoint. Otherwise 0 will be returned.
 * @return Returns 0 when glyph is not present or parsing is failed.
 */
uint16_t ttfp_get_glyph_index(const struct ttfp_face *face, uint32_t codepoint);

/**
 * @brief Resolves a variation of a Glyph ID from two code points.
 *
 * @param codepoint A valid Unicode codepoint. Otherwise 0 will be returned.
 * @param variation A valid Unicode codepoint. Otherwise 0 will be returned.
 * @return Returns 0 when glyph is not present or parsing is failed.
 */
uint16_t ttfp_get_glyph_var_index(const struct ttfp_face *face,
                                  uint32_t codepoint,
                                  uint32_t variation);

/**
 * @brief Returns glyph's horizontal advance.
 *
 * @return Glyph's advance or 0 when not set.
 */
uint16_t ttfp_get_glyph_hor_advance(const struct ttfp_face *face, uint16_t glyph_id);

/**
 * @brief Returns glyph's vertical advance.
 *
 * This function is affected by variation axes.
 *
 * @return Glyph's advance or 0 when not set.
 */
uint16_t ttfp_get_glyph_ver_advance(const struct ttfp_face *face, uint16_t glyph_id);

/**
 * @brief Returns glyph's horizontal side bearing.
 *
 * @return Glyph's side bearing or 0 when not set.
 */
int16_t ttfp_get_glyph_hor_side_bearing(const struct ttfp_face *face, uint16_t glyph_id);

/**
 * @brief Returns glyph's vertical side bearing.
 *
 * This function is affected by variation axes.
 *
 * @return Glyph's side bearing or 0 when not set.
 */
int16_t ttfp_get_glyph_ver_side_bearing(const struct ttfp_face *face, uint16_t glyph_id);

/**
 * @brief Returns glyph's vertical origin.
 *
 * @return Glyph's vertical origin or 0 when not set.
 */
int16_t ttfp_get_glyph_y_origin(const struct ttfp_face *face, uint16_t glyph_id);

/**
 * @brief Returns glyph's name.
 *
 * Uses the `post` and `CFF` tables as sources.
 *
 * A glyph name cannot be larger than 255 bytes + 1 byte for '\0'.
 *
 * @param name A char buffer larger than 256 bytes.
 * @return `true` on success.
 */
bool ttfp_get_glyph_name(const struct ttfp_face *face, uint16_t glyph_id, char *name);

/**
 * @brief Outlines a glyph and returns its tight bounding box.
 *
 * **Warning**: since `ttf-parser` is a pull parser,
 * `OutlineBuilder` will emit segments even when outline is partially malformed.
 * You must check #ttfp_outline_glyph() result before using
 * #ttfp_outline_builder 's output.
 *
 * `glyf`, `gvar`, `CFF` and `CFF2` tables are supported.
 *
 * This function is affected by variation axes.
 *
 * @return `false` when glyph has no outline or on error.
 */
bool ttfp_outline_glyph(const struct ttfp_face *face,
                        struct ttfp_outline_builder builder,
                        void *user_data,
                        uint16_t glyph_id,
                        struct ttfp_rect *bbox);

/**
 * @brief Returns a tight glyph bounding box.
 *
 * Unless the current face has a `glyf` table, this is just a shorthand for `outline_glyph()`
 * since only the `glyf` table stores a bounding box. In case of CFF and variable fonts
 * we have to actually outline a glyph to find it's bounding box.
 *
 * This function is affected by variation axes.
 */
bool ttfp_get_glyph_bbox(const struct ttfp_face *face, uint16_t glyph_id, struct ttfp_rect *bbox);

/**
 * @brief Returns a bounding box that large enough to enclose any glyph from the face.
 */
struct ttfp_rect ttfp_get_global_bounding_box(const struct ttfp_face *face);

/**
 * @brief Returns a reference to a glyph's raster image.
 *
 * A font can define a glyph using a raster or a vector image instead of a simple outline.
 * Which is primarily used for emojis. This method should be used to access raster images.
 *
 * `pixels_per_em` allows selecting a preferred image size. The chosen size will
 * be closer to an upper one. So when font has 64px and 96px images and `pixels_per_em`
 * is set to 72, 96px image will be returned.
 * To get the largest image simply use `SHRT_MAX`.
 *
 * Note that this method will return an encoded image. It should be decoded
 * by the caller. We don't validate or preprocess it in any way.
 *
 * Also, a font can contain both: images and outlines. So when this method returns `None`
 * you should also try `ttfp_outline_glyph()` afterwards.
 *
 * There are multiple ways an image can be stored in a TrueType font
 * and this method supports most of them.
 * This includes `sbix`, `bloc` + `bdat`, `EBLC` + `EBDT`, `CBLC` + `CBDT`.
 * And font's tables will be accesses in this specific order.
 */
bool ttfp_get_glyph_raster_image(const struct ttfp_face *face,
                                 uint16_t glyph_id,
                                 uint16_t pixels_per_em,
                                 struct ttfp_glyph_raster_image *glyph_image);

/**
 * @brief Returns a reference to a glyph's SVG image.
 *
 * A font can define a glyph using a raster or a vector image instead of a simple outline.
 * Which is primarily used for emojis. This method should be used to access SVG images.
 *
 * Note that this method will return just an SVG data. It should be rendered
 * or even decompressed (in case of SVGZ) by the caller.
 * We don't validate or preprocess it in any way.
 *
 * Also, a font can contain both: images and outlines. So when this method returns `false`
 * you should also try `ttfp_outline_glyph()` afterwards.
 */
bool ttfp_get_glyph_svg_image(const struct ttfp_face *face,
                              uint16_t glyph_id,
                              const char **svg,
                              uint32_t *len);

#if defined(TTFP_VARIABLE_FONTS)
/**
 * @brief Returns the amount of variation axes.
 */
uint16_t ttfp_get_variation_axes_count(const struct ttfp_face *face);
#endif

#if defined(TTFP_VARIABLE_FONTS)
/**
 * @brief Returns a variation axis by index.
 */
bool ttfp_get_variation_axis(const struct ttfp_face *face,
                             uint16_t index,
                             struct ttfp_variation_axis *axis);
#endif

#if defined(TTFP_VARIABLE_FONTS)
/**
 * @brief Returns a variation axis by tag.
 */
bool ttfp_get_variation_axis_by_tag(const struct ttfp_face *face,
                                    ttfp_tag tag,
                                    struct ttfp_variation_axis *axis);
#endif

#if defined(TTFP_VARIABLE_FONTS)
/**
 * @brief Sets a variation axis coordinate.
 *
 * This is the only mutable function in the library.
 * We can simplify the API a lot by storing the variable coordinates
 * in the face object itself.
 *
 * This function is reentrant.
 *
 * Since coordinates are stored on the stack, we allow only 32 of them.
 *
 * @return `false` when face is not variable or doesn't have such axis.
 */
bool ttfp_set_variation(struct ttfp_face *face, ttfp_tag axis, float value);
#endif

#if defined(TTFP_VARIABLE_FONTS)
/**
 * @brief Returns the current normalized variation coordinates.
 *
 * Values represented as f2.16
 */
const int16_t *ttfp_get_variation_coordinates(const struct ttfp_face *face);
#endif

#if defined(TTFP_VARIABLE_FONTS)
/**
 * @brief Checks that face has non-default variation coordinates.
 */
bool ttfp_has_non_default_variation_coordinates(const struct ttfp_face *face);
#endif

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  /* TTFP_H */
