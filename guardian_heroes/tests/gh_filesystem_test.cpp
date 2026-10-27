#include "gh_filesystem.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int main() {
  namespace fs = std::filesystem;
  const auto root = fs::temp_directory_path() / "guardian_heroes_gh_filesystem_test";
  const auto game = root / "game";
  const auto user = root / "profile";
  std::error_code error;
  fs::remove_all(root, error);
  fs::create_directories(game / "media");
  {
    const std::vector<uint8_t> expected{0, 0x47, 0x48, 0xFF};
    std::ofstream output(game / "media" / "sample.bin", std::ios::binary);
    output.write(reinterpret_cast<const char*>(expected.data()), expected.size());
  }

  gh::filesystem::Storage storage(game, user);
  assert(storage.game_data_root() == game);
  assert(storage.user_data_root() == user);
  std::vector<uint8_t> bytes;
  assert(storage.ReadResource("media/sample.bin", bytes));
  assert((bytes == std::vector<uint8_t>{0, 0x47, 0x48, 0xFF}));
  assert(!storage.ReadResource("missing.bin", bytes));
  assert(bytes.empty());
  assert(storage.WriteUserText("settings/language.txt", "zh-Hans\n"));
  std::string text;
  assert(storage.ReadUserText("settings/language.txt", text));
  assert(text == "zh-Hans\n");
  assert(storage.WriteUserText("settings/language.txt", "ja\n"));
  assert(storage.ReadUserText("settings/language.txt", text));
  assert(text == "ja\n");
  assert(!storage.ReadUserText("settings/missing.txt", text));
  assert(text.empty());
  assert(!fs::exists(game / "settings" / "language.txt"));

  {
    std::ofstream blocker(user / "blocked", std::ios::binary);
    blocker << "keep";
  }
  assert(!storage.WriteUserText("blocked/language.txt", "discard"));
  assert(storage.ReadUserText("blocked", text));
  assert(text == "keep");

  gh::filesystem::Storage default_user_root(game, {});
  assert(default_user_root.user_data_root() == game);
  fs::remove_all(root, error);
}
