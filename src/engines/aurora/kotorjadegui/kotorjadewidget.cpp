/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Common base for Star Wars: Knights of the Old Republic and Jade Empire widgets.
 */

#include "src/common/system.h"
#include "src/common/util.h"

#include "src/aurora/types.h"
#include "src/aurora/gff3file.h"
#include "src/aurora/talkman.h"

#include "src/graphics/font.h"
#include "src/graphics/windowman.h"

#include "src/graphics/aurora/fontman.h"
#include "src/graphics/aurora/guiquad.h"
#include "src/graphics/aurora/text.h"
#include "src/graphics/aurora/highlightabletext.h"
#include "src/graphics/aurora/highlightableguiquad.h"
#include "src/graphics/aurora/types.h"
#include "src/graphics/aurora/textureman.h"

#include "src/engines/aurora/kotorjadegui/kotorjadewidget.h"

namespace Engines {

KotORJadeWidget::Extend::Extend() : x(0.0f), y(0.0f), w(0.0f), h(0.0f) {
}

KotORJadeWidget::Border::Border()
		: fillStyle(0),
		  dimension(0),
		  innerOffset(0),
		  hasColor(false),
		  r(0.0f), g(0.0f), b(0.0f),
		  pulsing(false) {
}

KotORJadeWidget::Text::Text()
		: strRef(Aurora::kStrRefInvalid),
		  halign(0.0f),
		  valign(0.0f),
		  r(1.0f), g(1.0f), b(1.0f),
		  pulsing(false) {
}

KotORJadeWidget::Hilight::Hilight() : fill("") {
}

KotORJadeWidget::KotORJadeWidget(GUI &gui, const Common::UString &tag)
		: Widget(gui, tag),
		  _width(0.0f),
		  _height(0.0f),
		  _borderDimension(0),
		  _r(1.0f), _g(1.0f), _b(1.0f), _a(1.0f),
		  _unselectedR(1.0f), _unselectedG(1.0f), _unselectedB(1.0f), _unselectedA(1.0f),
		  _wrapped(false),
		  _subScene(NULL),
		  _highlighted(false) {

}

KotORJadeWidget::~KotORJadeWidget() {
}

void KotORJadeWidget::load(const Aurora::GFF3Struct &gff) {
	gff.getVector("COLOR", _r, _g, _b);
	_a = gff.getDouble("ALPHA", 1.0);

	Extend extend = createExtend(gff);

	_width  = extend.w;
	_height = extend.h;

	Widget::setPosition(extend.x, extend.y, 0.0f);

	Border border = createBorder(gff);
	Hilight hilight = createHilight(gff);

	if (!hilight.fill.empty()) {
		_highlight.reset(new Graphics::Aurora::GUIQuad(hilight.fill, 0, 0, extend.w, extend.h));
		_highlight->setPosition(extend.x, extend.y, 0.0f);
		_highlight->setTag(getTag());
		_highlight->setClickable(true);
	}

	if (!border.edge.empty() && !border.corner.empty()) {
		_border.reset(new Graphics::Aurora::BorderQuad(border.edge, border.corner, 0.0f, 0.0f, extend.w, extend.h, border.dimension));
		_border->setPosition(extend.x, extend.y, 0.0f);
		_border->setColor(border.r, border.g, border.b);
		if (!border.fill.empty()) {
			_quad.reset(new Graphics::Aurora::HighlightableGUIQuad(border.fill, 0.0f, 0.0f,
			                                                       extend.w - 2 * border.dimension,
			                                                       extend.h - 2 * border.dimension));
		} else {
			_quad.reset(new Graphics::Aurora::GUIQuad(border.fill, 0.0f, 0.0f,
			                                          extend.w - 2 * border.dimension,
			                                          extend.h - 2 * border.dimension));
		}
		_quad->setPosition(extend.x + border.dimension, extend.y + border.dimension, 0.0f);
	} else {
		if (!border.fill.empty()) {
			_quad.reset(new Graphics::Aurora::HighlightableGUIQuad(border.fill, 0.0f, 0.0f, extend.w, extend.h));
		} else {
			_quad.reset(new Graphics::Aurora::GUIQuad(border.fill, 0.0f, 0.0f, extend.w, extend.h));
		}
		_quad->setPosition(extend.x, extend.y, 0.0f);
	}

	_quad->setTag(getTag());
	_quad->setClickable(true);

	if (border.fill.empty())
		_quad->setColor(0.0f, 0.0f, 0.0f, 0.0f);
	else if (!border.hasColor)
		_quad->setColor(1.0f, 1.0f, 1.0f, 1.0f);
	else
		_quad->setColor(border.r, border.g, border.b, 1.0f);

	Text text = createText(gff);

	if (!text.font.empty()) {
		Graphics::Aurora::FontHandle font = FontMan.get(text.font);

		const float fontHeight = font.getFont().getHeight();
		float tY = extend.y, tH = extend.h;
		if (extend.h < fontHeight) {
			tH = fontHeight;
			tY = extend.y - (fontHeight - extend.h);
		}
		_text.reset(new Graphics::Aurora::HighlightableText(font, extend.w, tH,
		            text.text, text.r, text.g, text.b, 1.0f, text.halign, text.valign));

		_text->setPosition(extend.x, tY, -1.0f);
		_text->setTag(getTag());
		_text->setClickable(true);
	}

	_borderDimension = border.dimension;
}

void KotORJadeWidget::setClickable(bool clickable) {
	if (_quad)
		_quad->setClickable(clickable);
	if (_text)
		_text->setClickable(clickable);
	if (_highlight)
		_highlight->setClickable(clickable);
}

void KotORJadeWidget::setScissor(int x, int y, int width, int height) {
	if (_quad) {
		_quad->setScissor(true);
		_quad->setScissor(x, y, width, height);
	}
}

void KotORJadeWidget::setFill(const Common::UString &fill) {
	if (fill.empty() && _quad) {
		_quad->hide();
		_quad.reset();
		return;
	}

	if (!_quad) {
		float x, y, z;
		getPosition(x, y, z);

		_quad.reset(new Graphics::Aurora::GUIQuad("", 0.0f, 0.0f, _width - 2 * _borderDimension, _height - 2 * _borderDimension));
		_quad->setPosition(x + _borderDimension, y + _borderDimension, z);
		_quad->setTag(getTag());
		_quad->setClickable(true);

		if (isVisible())
			_quad->show();
	}

	_quad->setTexture(fill);
	_quad->setColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void KotORJadeWidget::setColor(float r, float g, float b, float a) {
	if (_quad)
		_quad->setColor(r, g, b, a);
}

void KotORJadeWidget::setBorderColor(float r, float g, float b, float a) {
	if (_border)
		_border->setColor(r, g, b, a);
}

void KotORJadeWidget::setWrapped(bool wrapped) {
	_wrapped = wrapped;
}

void KotORJadeWidget::setSubScene(Graphics::Aurora::SubSceneQuad *subscene) {
	if (!subscene) {
		_subScene = NULL;
		return;
	}

	_subScene = subscene;
	_subScene->setSize(_width, _height);

	float wWidth, wHeight;
	wWidth = WindowMan.getWindowWidth();
	wHeight = WindowMan.getWindowHeight();

	float x, y, z;
	getPosition(x, y, z);
	_subScene->setPosition(x + wWidth/2, y + wHeight/2);
}

float KotORJadeWidget::getBorderDimension() const {
	return _borderDimension;
}

void KotORJadeWidget::setHighlight(const Common::UString &hilight) {
	if (hilight.empty() && _highlight) {
		_highlight->hide();
		_highlight.reset();
		return;
	}

	if (!_highlight) {
		float x, y, z;
		getPosition(x, y, z);

		_highlight.reset(new Graphics::Aurora::GUIQuad("", 0.0f, 0.0f, _width - 2 * _borderDimension, _height - 2 * _borderDimension));
		_highlight->setPosition(x + _borderDimension, y + _borderDimension, z);
		_highlight->setTag(getTag());
		_highlight->setClickable(true);

		if (isVisible())
			_highlight->show();
	}

	_highlight->setTexture(hilight);
	_highlight->setColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void KotORJadeWidget::setHighlight(bool highlight) {
	float r, g, b, a;

	if (_highlighted == highlight)
		return;

	if (highlight) {
		if (_highlight) {
			_quad->hide();
			_highlight->show();
		} else {
			Graphics::Aurora::Highlightable *highlightable = getTextHighlightableComponent();
			if (highlightable && highlightable->isHighlightable()) {
				_text->getColor(_unselectedR, _unselectedG, _unselectedB, _unselectedA);
				_text->getHighlightedLowerBound(r, g, b, a);
				_text->setColor(r, g, b, a);
				_text->setHighlighted(true);
			}

			highlightable = getQuadHighlightableComponent();
			if (highlightable && highlightable->isHighlightable()) {
				_quad->getColor(_unselectedR, _unselectedG, _unselectedB, _unselectedA);
				highlightable->getHighlightedLowerBound(r, g, b, a);
				_quad->setColor(r, g, b, a);
				highlightable->setHighlighted(true);
			}
		}
	} else {
		if (_highlight) {
			_quad->show();
			_highlight->hide();
		} else {
			Graphics::Aurora::Highlightable *highlightable = getTextHighlightableComponent();
			if (highlightable && highlightable->isHighlightable()) {
				_text->setHighlighted(false);
				_text->setColor(_unselectedR, _unselectedG, _unselectedB, _unselectedA);
			}

			highlightable = getQuadHighlightableComponent();
			if (highlightable && highlightable->isHighlightable()) {
				highlightable->setHighlighted(false);
				_quad->setColor(_unselectedR, _unselectedG, _unselectedB, _unselectedA);
			}
		}
	}

	_highlighted = highlight;
}

bool KotORJadeWidget::isHighlight() {
	return _highlighted;
}

void KotORJadeWidget::createText(const Common::UString &font, const Common::UString &str) {
	const Graphics::Aurora::FontHandle f = FontMan.get(font);
	const float width  = f.getFont().getWidth(str);
	const float height = f.getFont().getHeight();

	_text.reset(new Graphics::Aurora::HighlightableText(f, width, height, str));

	_text->setTag(getTag());
	_text->setClickable(true);

	float x, y, z;
	getPosition(x, y, z);

	_text->setPosition(x, y, z);

	_width  = MAX(_width , width);
	_height = MAX(_height, height);
}

void KotORJadeWidget::setFont(const Common::UString &fnt) {
	if (_text)
		_text->setFont(fnt);
}

void KotORJadeWidget::setText(const Common::UString &text) {
	if (_text)
		_text->setText(text);
}

void KotORJadeWidget::setTextColor(float r, float g, float b, float a) {
	if (_text)
		_text->setColor(r, g, b, a);
}

void KotORJadeWidget::setHorizontalTextAlign(float halign) {
	if (_text)
		_text->setHorizontalAlign(halign);
}

void KotORJadeWidget::setVerticalTextAlign(float valign) {
	if (_text)
		_text->setVerticalAlign(valign);
}

float KotORJadeWidget::getTextHeight(const Common::UString &text) const {
	return _text ? _text->getHeight(text) : 0.0f;
}

void KotORJadeWidget::setTag(const Common::UString &tag) {
	Widget::setTag(tag);

	if (_quad)
		_quad->setTag(getTag());
	if (_text)
		_text->setTag(getTag());
}

void KotORJadeWidget::show() {
	if (isInvisible())
		return;

	Widget::show();

	if (_quad)
		_quad->show();
	if (_text)
		_text->show();
	if (_border)
		_border->show();
	if (_subScene)
		_subScene->show();
}

void KotORJadeWidget::hide() {
	if (!isVisible())
		return;

	if (_subScene)
		_subScene->hide();
	if (_border)
		_border->hide();
	if (_highlight)
		_highlight->hide();
	if (_quad)
		_quad->hide();
	if (_text)
		_text->hide();

	Widget::hide();
}

void KotORJadeWidget::setPosition(float x, float y, float z) {
	float oX, oY, oZ;
	getPosition(oX, oY, oZ);

	float dx = x - oX;
	float dy = y - oY;
	float dz = z - oZ;

	Widget::setPosition(x, y, z);

	if (_quad) {
		_quad->getPosition(x, y, z);
		_quad->setPosition(x + dx, y + dy, z + dz);
	}

	if (_highlight) {
		_highlight->getPosition(x, y, z);
		_highlight->setPosition(x + dx, y + dy, z + dz);
	}

	if (_text) {
		_text->getPosition(x, y, z);
		_text->setPosition(x + dx, y + dy, z + dz);
	}

	if (_border) {
		_border->getPosition(x, y, z);
		_border->setPosition(x + dx, y + dy, z + dz);
	}
}

void KotORJadeWidget::setWidth(float width) {
	float deltaWidth = width - _width;
	_width = width;

	if (_quad) {
		width = _quad->getWidth();
		_quad->setWidth(width + deltaWidth);
	}

	if (_highlight) {
		width = _highlight->getWidth();
		_highlight->setWidth(width + deltaWidth);
	}

	float height;

	if (_text) {
		width = _text->getWidth();
		height = _text->getHeight();
		_text->setSize(width + deltaWidth, height);
	}

	if (_border) {
		_border->getSize(width, height);
		_border->setSize(width + deltaWidth, height);
	}
}

void KotORJadeWidget::setHeight(float height) {
	float deltaHeight = height - _height;
	_height = height;

	if (_quad) {
		height = _quad->getHeight();
		_quad->setHeight(height + deltaHeight);
	}

	if (_highlight) {
		height = _highlight->getHeight();
		_highlight->setHeight(height + deltaHeight);
	}

	float width;

	if (_text) {
		width = _text->getWidth();
		height = _text->getHeight();
		_text->setSize(width, height + deltaHeight);
	}

	if (_border) {
		_border->getSize(width, height);
		_border->setSize(width, height + deltaHeight);
	}
}

float KotORJadeWidget::getWidth() const {
	return _width;
}

float KotORJadeWidget::getHeight() const {
	return _height;
}

void KotORJadeWidget::setInvisible(bool invisible) {
	Widget::setInvisible(invisible);
	setClickable(!invisible);
}

Graphics::Aurora::Highlightable *KotORJadeWidget::getTextHighlightableComponent() const {
	return static_cast<Graphics::Aurora::Highlightable *>(_text.get());
}

Graphics::Aurora::Highlightable *KotORJadeWidget::getQuadHighlightableComponent() const {
	return dynamic_cast<Graphics::Aurora::Highlightable *>(_quad.get());
}

KotORJadeWidget::Extend KotORJadeWidget::createExtend(const Aurora::GFF3Struct &gff) {
	Extend extend;
	if (gff.hasField("EXTENT")) {
		const Aurora::GFF3Struct &e = gff.getStruct("EXTENT");
		extend.x = static_cast<float>(e.getSint("LEFT"));
		extend.y = static_cast<float>(e.getSint("TOP"));
		extend.w = static_cast<float>(e.getSint("WIDTH"));
		extend.h = static_cast<float>(e.getSint("HEIGHT"));
	}
	return extend;
}

KotORJadeWidget::Border KotORJadeWidget::createBorder(const Aurora::GFF3Struct &gff) {
	Border border;

	if (gff.hasField("BORDER")) {
		const Aurora::GFF3Struct &b = gff.getStruct("BORDER");

		border.corner = b.getString("CORNER");
		border.edge   = b.getString("EDGE");
		border.fill   = b.getString("FILL");

		border.fillStyle   = b.getUint("FILLSTYLE");
		border.dimension   = b.getUint("DIMENSION");
		border.innerOffset = b.getUint("INNEROFFSET");

		border.hasColor = b.hasField("COLOR");
		b.getVector("COLOR", border.r, border.g, border.b);

		border.pulsing = b.getBool("PULSING");
	}
	return border;
}

KotORJadeWidget::Text KotORJadeWidget::createText(const Aurora::GFF3Struct &gff) {
	Text text;

	if (gff.hasField("TEXT")) {
		const Aurora::GFF3Struct &t = gff.getStruct("TEXT");

		text.font   = t.getString("FONT");
		text.text   = t.getString("TEXT");
		text.strRef = t.getUint("STRREF", Aurora::kStrRefInvalid);

		const uint32 alignment = t.getUint("ALIGNMENT");

		t.getVector("COLOR", text.r, text.g, text.b);

		text.pulsing = t.getBool("PULSING");


		if (text.text == "(Unitialized)")
			text.text.clear();

		if (text.strRef != Aurora::kStrRefInvalid)
			text.text = TalkMan.getString(text.strRef);

		// TODO: KotORWidget::getText(): Alignment
		if (alignment == 10) {
			text.halign = Graphics::Aurora::kHAlignCenter;
			text.valign = Graphics::Aurora::kVAlignTop;
		} else if (alignment == 18) {
			text.halign = Graphics::Aurora::kHAlignCenter;
			text.valign = Graphics::Aurora::kVAlignMiddle;
		}
	}

	return text;
}

KotORJadeWidget::Hilight KotORJadeWidget::createHilight(const Aurora::GFF3Struct &gff) {
	Hilight hilight;
	if (gff.hasField("HILIGHT")) {
		const Aurora::GFF3Struct &h = gff.getStruct("HILIGHT");
		hilight.fill = h.getString("FILL");
	}
	return hilight;
}

} // End of namespace Engines
