#pragma once
#include <memory>
namespace drayven { class EditorApp { public: EditorApp(); ~EditorApp(); EditorApp(const EditorApp&)=delete; EditorApp& operator=(const EditorApp&)=delete; int run(); private: struct Impl; std::unique_ptr<Impl> m_impl; }; }
