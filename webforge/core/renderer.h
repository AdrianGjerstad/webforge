// Copyright (C) 2025 Adrian Gjerstad
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------
// File: renderer.h
// -----------------------------------------------------------------------------
//
// The wf::Renderer class is the centerpiece of WebForge's build pipeline. It
// uses the [Inja](https://github.com/pantor/inja) Template Engine, which is
// "loosely inspired by jinja for python." The original purpose of Jinja2, to
// which it refers, was to render HTML components into complete documents. Inja,
// however, is sufficiently genericized that we can use it on any file type.
//
// The wf::Renderer class acts as a mature wrapper around inja::Environment,
// which manages templates, optional HTML autoescaping, and piping
// automatically. This class is designed to exist for the lifetime of whatever
// program that uses it, as inja::Template objects are cached for future reuse
// (unless explicitly Flush'd).
//

#ifndef WEBFORGE_CORE_RENDERER_H_
#define WEBFORGE_CORE_RENDERER_H_

#include <string>

#include <inja/inja.hpp>

namespace wf {

class Renderer {
public:
  Renderer();

  //std::string RenderComponent(const std::string& filename);

private:
  inja::Environment env_;
};

}

#endif  // WEBFORGE_CORE_RENDERER_H_

