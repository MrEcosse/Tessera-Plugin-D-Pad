#include "d_pad.h"
#include "dpad_command.h"
#include "esphome/core/hal.h"
#include <algorithm>
#include <cstring>

namespace esphome::d_pad {

using tessera::Font;
using tessera::Tone;
namespace ui = tessera::ui;

// The keys by their option in the manifest and the place each has in the 3 x 3 square (0 is the top left, 8 the
// bottom right): the arrows round the OK key, and the four corners for the keys a remote has beside them. Their icons
// are Material Design Icons of Tessera's set.
struct KeyKind {
  const char *option;
  uint32_t glyph;
  int slot;
};
static constexpr KeyKind KINDS[] = {
    {"back", 0xF17B3, 0},   // arrow-u-left-top
    {"up", 0xF0143, 1},     // chevron-up
    {"home", 0xF02DC, 2},   // home
    {"left", 0xF0141, 3},   // chevron-left
    {"ok", 0xF012C, 4},     // check
    {"right", 0xF0142, 5},  // chevron-right
    {"menu", 0xF0279, 6},   // format-list-bulleted
    {"down", 0xF0140, 7},   // chevron-down
    {"guide", 0xF0502, 8},  // television
};
static constexpr int KIND_COUNT = sizeof(KINDS) / sizeof(KINDS[0]);

// How long a key stays lit after a tap: at least this long, and until the next tick after it.
static constexpr uint32_t FLASH_MS = 300;

// The tile: the keys that have an action, in a square of three by three in the middle of the card. A key that has no
// action leaves its place empty, so the arrows keep their places whatever else is there.
class PadTile : public tessera::Tile {
 public:
  explicit PadTile(const tessera::Plugin *plugin) : plugin_(plugin) {
    for (int &s : slot_key_) s = -1;
  }

  void create(const tessera::TileContext &c) override {
    const char *entity = c.entity ? c.entity : "";
    detail::copy(entity_, sizeof entity_, entity, std::strlen(entity));

    for (int k = 0; k < KIND_COUNT; ++k) {
      Key &key = keys_[count_];
      const Parsed parsed = parse(c.options[KINDS[k].option] | "", key.command);
      if (parsed == Parsed::EMPTY) continue;
      key.kind = k;
      key.valid = parsed == Parsed::OK;
      slot_key_[KINDS[k].slot] = count_;
      ++count_;
    }

    if (count_ == 0) {
      // Nothing to press yet: say what to do, in the middle.
      hint_ = ui::label(c.parent, Font::BODY, theme::MUTED);
      lv_obj_set_width(hint_, c.width);
      lv_obj_set_style_text_align(hint_, LV_TEXT_ALIGN_CENTER, 0);
      ui::set_text(hint_, plugin_->text("no_keys"));
      lv_obj_set_pos(hint_, 0, (c.height - ui::line_height(Font::BODY)) / 2);
      return;
    }

    // Keys as large as the card allows but no more than 14 mm high, and no wider than half again their height: a
    // finger's key, not a slab on a large tile. A wide tile gives them width, a tall one only space round them. The gap
    // narrows before the keys do.
    gap_ = ui::px(8);
    kh_ = (c.height - 2 * gap_) / 3;
    kw_ = (c.width - 2 * gap_) / 3;
    if (std::min(kh_, kw_) < ui::mm(9)) {
      gap_ = ui::px(3);
      kh_ = (c.height - 2 * gap_) / 3;
      kw_ = (c.width - 2 * gap_) / 3;
    }
    kh_ = std::max(1, std::min(kh_, ui::mm(14)));
    kw_ = std::max(1, std::min(kw_, kh_ * 3 / 2));
    left_ = (c.width - (3 * kw_ + 2 * gap_)) / 2;
    top_ = (c.height - (3 * kh_ + 2 * gap_)) / 2;
    const int radius = std::min(ui::px(ui::large() ? 12 : 8), std::min(kw_, kh_) / 2);

    // The largest icon that leaves the key a margin.
    const int room = std::min(kw_, kh_) * 3 / 4;
    const Font face = ui::text_width(ui::icon(0xF040A), Font::ICON) <= room ? Font::ICON : Font::ICON_SMALL;
    const int line = ui::line_height(face);

    for (int i = 0; i < count_; ++i) {
      Key &key = keys_[i];
      const int slot = KINDS[key.kind].slot;
      key.block = ui::block(c.parent, theme::BUTTON);
      lv_obj_set_size(key.block, kw_, kh_);
      lv_obj_set_pos(key.block, left_ + (slot % 3) * (kw_ + gap_), top_ + (slot / 3) * (kh_ + gap_));
      lv_obj_set_style_radius(key.block, radius, 0);
      key.icon = ui::label(key.block, face, theme::INK);
      lv_obj_set_width(key.icon, kw_);
      lv_obj_set_style_text_align(key.icon, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_pos(key.icon, 0, (kh_ - line) / 2);
      ui::set_text(key.icon, ui::icon(KINDS[key.kind].glyph));
    }
    paint();
  }

  // The entity's state: a remote or player that is not there greys the keys.
  void on_state(JsonObjectConst data) override {
    const char *state = data["state"] | "";
    if (std::strcmp(state, state_) == 0) return;
    detail::copy(state_, sizeof state_, state, std::strlen(state));
    paint();
  }

  void on_tick(uint32_t) override {
    if (flash_ >= 0 && esphome::millis() - flash_at_ >= FLASH_MS) {
      flash_ = -1;
      paint();
    }
  }

  void on_theme() override {
    if (hint_) ui::set_color(hint_, theme::MUTED);
    paint();
  }

  void on_tap() override {
    const int i = key_at(tessera::tap_point());
    if (i < 0) return;
    const Key &key = keys_[i];
    const char *target = key.command.target[0] ? key.command.target : entity_;
    bool sent = false;
    if (key.valid && target[0])
      sent = tessera::action(key.command.action, target, key.command.key[0] ? key.command.key : nullptr,
                             key.command.value);
    flash_ = i;
    flash_ok_ = sent;
    flash_at_ = esphome::millis();
    paint();
  }

 private:
  struct Key {
    int kind = 0;
    bool valid = false;
    Command command;
    lv_obj_t *block = nullptr, *icon = nullptr;
  };

  // The key under the finger: the place it stands in and half the gap around it, so a tap between two keys still
  // counts for the nearer one. A place with no key gives -1; a tile with a single key gives it whenever the point is
  // unknown.
  int key_at(tessera::Point p) const {
    if (count_ == 0) return -1;
    if (p.x < 0 || p.y < 0) return count_ == 1 ? 0 : -1;
    const int x = p.x - left_ + gap_ / 2, y = p.y - top_ + gap_ / 2;
    if (x < 0 || y < 0) return -1;
    const int col = x / (kw_ + gap_), row = y / (kh_ + gap_);
    if (col > 2 || row > 2) return -1;
    return slot_key_[row * 3 + col];
  }

  void paint() {
    const bool away = std::strcmp(state_, "unavailable") == 0;
    for (int i = 0; i < count_; ++i) {
      Key &key = keys_[i];
      // A tap: the pressed blue while it goes out, red when it could not be sent.
      if (i == flash_) {
        if (flash_ok_) ui::set_fill(key.block, Tone::NORMAL, theme::ACCENT_PRESSED);
        else ui::set_fill(key.block, Tone::ALERT, theme::BUTTON);
        ui::set_color(key.icon, theme::ON_ACCENT);
        continue;
      }
      ui::set_fill(key.block, Tone::NORMAL, theme::BUTTON);
      if (!key.valid) ui::set_tone(key.icon, Tone::ALERT, theme::INK);
      else if (away) ui::set_color(key.icon, theme::SUBTLE);
      else ui::set_color(key.icon, theme::INK);
    }
  }

  const tessera::Plugin *plugin_;
  Key keys_[KIND_COUNT];
  int slot_key_[9];
  int count_ = 0;
  int kw_ = 1, kh_ = 1, gap_ = 0, left_ = 0, top_ = 0;
  char entity_[128] = "";
  char state_[32] = "";
  int flash_ = -1;
  bool flash_ok_ = false;
  uint32_t flash_at_ = 0;
  lv_obj_t *hint_ = nullptr;
};

void DPad::setup() {
  add_tile("pad", [this]() { return new PadTile(this); });
}

}  // namespace esphome::d_pad
