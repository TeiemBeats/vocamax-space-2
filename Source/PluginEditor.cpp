/*
    VocaMax Space 2 - editor
    By Mark33s Beats Lab

    Custom drawn glassy royal blue interface: starfield, planet, waveform,
    four glass knobs, stereo meters, vocal focus buttons and preset controls.
*/

#include "PluginEditor.h"

#include <cmath>
#include <functional>

namespace
{
namespace palette
{
    const juce::Colour background  { 0xff03060d };
    const juce::Colour royalLight  { 0xff1a4bb0 };
    const juce::Colour sky         { 0xff35c8ff };
    const juce::Colour skySoft     { 0xff9fe8ff };
    const juce::Colour ice         { 0xffeaf8ff };
}

juce::Font uiFont (float height, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (height, style));
}

/*  Letter spaced label, vertically centred in its area. */
void drawTrackedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                      juce::Colour colour, float fontHeight, float tracking = 0.14f,
                      juce::Justification justification = juce::Justification::centred,
                      int style = juce::Font::plain)
{
    if (area.getWidth() < 4.0f || area.getHeight() < 2.0f)
        return;

    juce::AttributedString attributed;
    attributed.setJustification (justification);
    attributed.setColour (colour);
    attributed.setExtraKerningFactor (tracking);
    attributed.append (text, uiFont (fontHeight, style), colour);

    juce::TextLayout layout;
    layout.createLayout (attributed, area.getWidth());
    const float textHeight = (float) layout.getHeight();
    layout.draw (g, area.withY (area.getY() + (area.getHeight() - textHeight) * 0.5f));
}

/*  Gradient filled text via glyph paths (wordmark). */
void drawGradientText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                       const juce::Font& font, juce::Colour leftColour, juce::Colour rightColour)
{
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText (font, text, 0.0f, 0.0f);

    juce::Path path;
    glyphs.createPath (path);

    const auto bounds = path.getBounds();
    if (bounds.isEmpty())
        return;

    const float x = area.getX() - bounds.getX();
    const float y = area.getCentreY() - bounds.getHeight() * 0.5f - bounds.getY();
    path.applyTransform (juce::AffineTransform::translation (x, y));

    const auto placed = path.getBounds();

    for (int i = 3; i >= 1; --i)
    {
        g.setColour (rightColour.withAlpha (0.05f));
        g.fillPath (path, juce::AffineTransform::translation (0.0f, (float) i));
    }

    juce::ColourGradient gradient (leftColour, placed.getX(), 0.0f,
                                   rightColour, placed.getRight(), 0.0f, false);
    g.setGradientFill (gradient);
    g.fillPath (path);
}

void drawCrown (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float w = area.getWidth();
    const float h = area.getHeight();

    juce::Path crown;
    crown.startNewSubPath (0.02f, 0.72f);
    crown.lineTo (0.02f, 0.26f);
    crown.lineTo (0.27f, 0.50f);
    crown.lineTo (0.42f, 0.10f);
    crown.lineTo (0.58f, 0.48f);
    crown.lineTo (0.80f, 0.22f);
    crown.lineTo (0.80f, 0.72f);
    crown.closeSubPath();
    crown.applyTransform (juce::AffineTransform::scale (w, h).translated (area.getX(), area.getY()));

    g.setColour (palette::sky.withAlpha (0.22f));
    g.fillPath (crown, juce::AffineTransform::translation (0.0f, 1.6f));

    juce::ColourGradient metal (palette::ice, area.getX(), area.getY(),
                                palette::sky, area.getRight(), area.getBottom(), false);
    g.setGradientFill (metal);
    g.fillPath (crown);

    g.setColour (palette::ice);
    const float gem = h * 0.075f;

    for (float fx : { 0.02f, 0.42f, 0.80f })
        g.fillEllipse (area.getX() + w * fx - gem, area.getY() + h * 0.10f - gem, gem * 2.0f, gem * 2.0f);
}

/*  Faint waveform across the glass panel. */
void drawPanelWaveform (juce::Graphics& g, juce::Rectangle<float> panel, float baseline)
{
    juce::Path line;
    bool started = false;

    for (float x = panel.getX(); x <= panel.getRight(); x += 2.5f)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (x - panel.getX()) / panel.getWidth());
        const float envelope = std::pow (std::sin (juce::MathConstants<float>::pi * t), 1.5f);
        const float wiggle = 0.55f * std::sin (t * 41.0f + 0.7f)
                           + 0.30f * std::sin (t * 97.0f + 2.1f)
                           + 0.15f * std::sin (t * 213.0f + 0.4f);
        const float amplitude = panel.getHeight() * 0.155f * envelope * (0.22f + 0.78f * std::abs (wiggle));
        const float y = baseline + wiggle * amplitude;

        if (! started)
        {
            line.startNewSubPath (x, y);
            started = true;
        }
        else
        {
            line.lineTo (x, y);
        }
    }

    if (! started)
        return;

    juce::Path filled (line);
    filled.lineTo (panel.getRight(), baseline + panel.getHeight() * 0.02f);
    filled.lineTo (panel.getX(), baseline + panel.getHeight() * 0.02f);
    filled.closeSubPath();

    juce::ColourGradient fill (palette::sky.withAlpha (0.10f), panel.getX(), baseline - panel.getHeight() * 0.18f,
                               palette::sky.withAlpha (0.0f), panel.getX(), baseline + panel.getHeight() * 0.16f, false);
    g.setGradientFill (fill);
    g.fillPath (filled);

    g.setColour (palette::sky.withAlpha (0.10f));
    g.strokePath (line, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (palette::sky.withAlpha (0.38f));
    g.strokePath (line, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
} // namespace

//==============================================================================
class VocaMaxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VocaMaxLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, palette::background);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff07152c));
        setColour (juce::PopupMenu::textColourId, juce::Colours::white.withAlpha (0.9f));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, palette::royalLight.withAlpha (0.85f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff07152c));
        setColour (juce::ComboBox::textColourId, palette::skySoft);
        setColour (juce::ComboBox::outlineColourId, palette::sky.withAlpha (0.32f));
        setColour (juce::ComboBox::arrowColourId, palette::sky);
        setColour (juce::Label::textColourId, juce::Colours::white);
        setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff07152c));
        setColour (juce::TooltipWindow::textColourId, palette::skySoft);
        setColour (juce::TooltipWindow::outlineColourId, palette::sky.withAlpha (0.4f));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        jassert (width > 0 && height > 0);

        auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
        const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        if (radius < 8.0f)
            return;

        const auto centre = area.getCentre();
        const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Soft outer glow.
        {
            juce::ColourGradient glow (accent.withAlpha (0.20f), centre.x, centre.y,
                                       accent.withAlpha (0.0f), centre.x, centre.y - radius * 1.08f, true);
            g.setGradientFill (glow);
            g.fillEllipse (juce::Rectangle<float> (radius * 2.3f, radius * 2.3f).withCentre (centre));
        }

        // Tick ring.
        const int tickCount = 60;
        for (int i = 0; i <= tickCount; ++i)
        {
            const float t = (float) i / (float) tickCount;
            const float a = juce::jmap (t, rotaryStartAngle - 0.12f, rotaryEndAngle + 0.12f);
            const bool major = (i % 5 == 0);
            const float inner = radius * (major ? 0.900f : 0.930f);
            const float outer = radius * (major ? 0.982f : 0.958f);
            const juce::Point<float> direction (std::sin (a), -std::cos (a));

            g.setColour (juce::Colours::white.withAlpha (major ? 0.34f : 0.14f));
            g.drawLine ({ centre + direction * inner, centre + direction * outer }, major ? 1.25f : 0.8f);
        }

        // Metallic bezel ring.
        const float ringThickness = juce::jmax (3.0f, radius * 0.105f);
        const auto outerRing = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);

        {
            juce::Path ringPath;
            ringPath.addEllipse (outerRing.reduced (ringThickness * 0.5f));
            juce::ColourGradient bezel (juce::Colour (0xffdceeff), centre.x - radius * 0.55f, centre.y - radius * 0.75f,
                                        juce::Colour (0xff0a1322), centre.x + radius * 0.45f, centre.y + radius * 0.65f, false);
            g.setGradientFill (bezel);
            g.strokePath (ringPath, juce::PathStrokeType (ringThickness, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        }

        // Glass body.
        const auto body = outerRing.reduced (ringThickness * 1.85f);
        {
            juce::ColourGradient bodyFill (juce::Colour (0xff22344f), centre.x - radius * 0.25f, body.getY(),
                                           juce::Colour (0xff03060d), centre.x, body.getBottom(), false);
            g.setGradientFill (bodyFill);
            g.fillEllipse (body);

            juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.12f),
                                        centre.x - radius * 0.28f, centre.y - radius * 0.46f,
                                        juce::Colours::white.withAlpha (0.0f),
                                        centre.x + radius * 0.10f, centre.y + radius * 0.25f, true);
            g.setGradientFill (sheen);
            g.fillEllipse (body.reduced (1.0f));

            g.setColour (juce::Colours::white.withAlpha (0.16f));
            g.drawEllipse (body.reduced (0.8f), 1.0f);
        }

        // Value arc.
        if (std::abs (angle - rotaryStartAngle) > 0.002f)
        {
            juce::Path arc;
            arc.addCentredArc (centre.x, centre.y, radius * 0.855f, radius * 0.855f,
                               0.0f, rotaryStartAngle, angle, true);

            g.setColour (accent.withAlpha (0.16f));
            g.strokePath (arc, juce::PathStrokeType (ringThickness * 2.2f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            g.setColour (accent.withAlpha (0.95f));
            g.strokePath (arc, juce::PathStrokeType (juce::jmax (2.0f, ringThickness * 0.62f),
                                                     juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Pointer.
        {
            const float pointerWidth = juce::jmax (2.6f, radius * 0.055f);
            juce::Path pointer;
            pointer.addRoundedRectangle (-pointerWidth * 0.5f, -radius * 0.66f,
                                         pointerWidth, radius * 0.56f, pointerWidth * 0.5f);
            pointer.applyTransform (juce::AffineTransform::rotation (angle, centre.x, centre.y));

            g.setColour (accent.withAlpha (0.30f));
            g.strokePath (pointer, juce::PathStrokeType (pointerWidth * 2.4f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
            g.setColour (juce::Colours::white.withAlpha (0.95f));
            g.fillPath (pointer);
        }

        // Hub.
        {
            const float hubRadius = radius * 0.155f;
            juce::ColourGradient hub (juce::Colour (0xff28405f), centre.x - hubRadius * 0.5f, centre.y - hubRadius * 0.5f,
                                      juce::Colour (0xff04070e), centre.x + hubRadius * 0.5f, centre.y + hubRadius * 0.5f, true);
            g.setGradientFill (hub);
            g.fillEllipse (juce::Rectangle<float> (hubRadius * 2.0f, hubRadius * 2.0f).withCentre (centre));

            g.setColour (accent.withAlpha (0.85f));
            g.drawEllipse (juce::Rectangle<float> (hubRadius * 1.55f, hubRadius * 1.55f).withCentre (centre), 1.2f);
            g.setColour (accent.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (hubRadius * 0.55f, hubRadius * 0.55f).withCentre (centre));
        }
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override
    {
        juce::ignoreUnused (buttonX, buttonY, buttonW, buttonH);

        auto b = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        const float corner = 7.0f;

        g.setColour (juce::Colour (0xff07152c).withAlpha (isButtonDown ? 0.98f : 0.82f));
        g.fillRoundedRectangle (b, corner);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (b, corner, 1.0f);

        if (box.hasKeyboardFocus (false))
        {
            g.setColour (palette::sky.withAlpha (0.22f));
            g.drawRoundedRectangle (b.expanded (2.0f), corner + 2.0f, 2.0f);
        }

        g.setColour (palette::sky.withAlpha (0.85f));
        juce::Path chevron;
        const float cx = b.getRight() - 15.0f;
        const float cy = b.getCentreY();
        chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
        chevron.lineTo (cx, cy + 2.6f);
        chevron.lineTo (cx + 4.0f, cy - 2.0f);
        g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

        g.setColour (box.findColour (juce::ComboBox::textColourId));
        g.setFont (getComboBoxFont (box));
        g.drawText (box.getText(), b.reduced (12.0f, 0.0f).withTrimmedRight (20.0f),
                    juce::Justification::centredLeft, true);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return uiFont (12.5f);
    }
};

//==============================================================================
class VocaMaxIconButton : public juce::Button
{
public:
    enum class IconType { ChevronLeft, ChevronRight, Save, Settings, Help };

    VocaMaxIconButton (IconType typeToUse, const juce::String& tooltip)
        : juce::Button (tooltip), type (typeToUse)
    {
        setTooltip (tooltip);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.0f);
        const float corner = 7.0f;

        g.setColour (juce::Colour (0xff0a1a35).withAlpha (down ? 0.98f : (highlighted ? 0.9f : 0.65f)));
        g.fillRoundedRectangle (b, corner);

        g.setColour (palette::sky.withAlpha (highlighted ? 0.7f : 0.28f));
        g.drawRoundedRectangle (b.reduced (0.5f), corner, 1.0f);

        drawIcon (g, b, highlighted ? juce::Colours::white : palette::skySoft);
    }

private:
    void drawIcon (juce::Graphics& g, juce::Rectangle<float> b, juce::Colour colour)
    {
        g.setColour (colour);
        const auto centre = b.getCentre();

        switch (type)
        {
            case IconType::ChevronLeft:
            case IconType::ChevronRight:
            {
                const float direction = (type == IconType::ChevronLeft ? -1.0f : 1.0f);
                juce::Path chevron;
                chevron.startNewSubPath (centre.x - 2.6f * direction, centre.y - 5.0f);
                chevron.lineTo (centre.x + 2.6f * direction, centre.y);
                chevron.lineTo (centre.x - 2.6f * direction, centre.y + 5.0f);
                g.strokePath (chevron, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved,
                                                             juce::PathStrokeType::rounded));
                break;
            }

            case IconType::Save:
            {
                auto body = b.reduced (b.getWidth() * 0.30f, b.getHeight() * 0.27f);
                juce::Path outline;
                outline.addRoundedRectangle (body, 2.0f);
                g.strokePath (outline, juce::PathStrokeType (1.4f));
                g.fillRect (body.getX() + body.getWidth() * 0.30f, body.getY() + 1.6f,
                            body.getWidth() * 0.40f, body.getHeight() * 0.22f);
                g.fillRect (body.getX() + body.getWidth() * 0.22f, body.getBottom() - body.getHeight() * 0.36f,
                            body.getWidth() * 0.56f, body.getHeight() * 0.30f);
                break;
            }

            case IconType::Settings:
            {
                const float radius = b.getWidth() * 0.24f;

                for (int i = 0; i < 8; ++i)
                {
                    const float a = juce::MathConstants<float>::twoPi * (float) i / 8.0f;
                    const juce::Point<float> direction (std::sin (a), -std::cos (a));
                    g.drawLine ({ centre + direction * (radius * 0.82f), centre + direction * (radius * 1.35f) }, 1.8f);
                }

                g.drawEllipse (juce::Rectangle<float> (radius * 1.55f, radius * 1.55f).withCentre (centre), 1.6f);
                g.fillEllipse (juce::Rectangle<float> (radius * 0.48f, radius * 0.48f).withCentre (centre));
                break;
            }

            case IconType::Help:
            {
                g.setFont (uiFont (b.getHeight() * 0.62f, juce::Font::bold));
                g.drawText ("?", b, juce::Justification::centred, false);
                break;
            }
        }
    }

    IconType type;
};

//==============================================================================
class VocaMaxKnobPanel : public juce::Component
{
public:
    VocaMaxKnobPanel (const juce::String& titleText, const juce::String& subtitleText,
                      const juce::String& leftText, const juce::String& rightText,
                      juce::Colour accentColour, bool isLarge, float defaultValue)
        : title (titleText), subtitle (subtitleText), leftCaption (leftText), rightCaption (rightText),
          accent (accentColour), large (isLarge)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                    juce::MathConstants<float>::pi * 2.8f, true);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRange (0.0, 100.0, 0.1);
        slider.setDoubleClickReturnValue (true, (double) defaultValue);
        slider.setMouseDragSensitivity (170);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
        slider.setName (titleText);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
    }

    juce::Slider& getSlider() { return slider; }

    void resized() override
    {
        auto area = getLocalBounds().toFloat();
        area.removeFromTop (titleHeight());
        area.removeFromBottom (footerHeight());

        const float diameter = juce::jmin (area.getWidth(), area.getHeight());
        slider.setBounds (area.withSizeKeepingCentre (diameter, diameter).toNearestInt());
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        auto titleArea = area.removeFromTop (titleHeight());

        drawTrackedText (g, title, titleArea, juce::Colours::white.withAlpha (0.93f),
                         large ? 13.5f : 11.8f, 0.15f, juce::Justification::centred, juce::Font::bold);

        auto footer = area.removeFromBottom (footerHeight());
        auto captions = footer.removeFromTop (14.0f);

        drawTrackedText (g, leftCaption, captions.withWidth (captions.getWidth() * 0.5f),
                         juce::Colours::white.withAlpha (0.42f), 8.6f, 0.08f, juce::Justification::left);
        drawTrackedText (g, rightCaption, captions.withTrimmedLeft (captions.getWidth() * 0.5f),
                         juce::Colours::white.withAlpha (0.42f), 8.6f, 0.08f, juce::Justification::right);

        auto valueArea = footer.removeFromTop (large ? 26.0f : 22.0f);
        drawTrackedText (g, juce::String (juce::roundToInt (slider.getValue())) + "%",
                         valueArea, accent.brighter (0.15f), large ? 18.0f : 14.5f, 0.05f,
                         juce::Justification::centred, juce::Font::bold);

        drawTrackedText (g, subtitle, footer, juce::Colours::white.withAlpha (0.52f),
                         8.6f, 0.16f, juce::Justification::centred);
    }

private:
    float titleHeight() const { return large ? 26.0f : 22.0f; }
    float footerHeight() const { return large ? 74.0f : 70.0f; }

    juce::Slider slider;
    juce::String title, subtitle, leftCaption, rightCaption;
    juce::Colour accent;
    bool large;
};

//==============================================================================
class VocaMaxLevelMeter : public juce::Component
{
public:
    VocaMaxLevelMeter (juce::String captionText, juce::Colour accentColour,
                       std::atomic<float>& leftSource, std::atomic<float>& rightSource)
        : caption (std::move (captionText)), accent (accentColour),
          leftLevel (leftSource), rightLevel (rightSource)
    {
        setInterceptsMouseClicks (false, false);
    }

    void update()
    {
        const float level = juce::jmax (leftLevel.load (std::memory_order_relaxed),
                                        rightLevel.load (std::memory_order_relaxed));
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float normalised = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);

        displayed = juce::jmax (normalised, displayed * 0.88f);
        peak = juce::jmax (normalised, peak - 0.006f);
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        auto captionArea = area.removeFromTop (14.0f);
        auto ticksArea = area.removeFromBottom (10.0f);
        area.reduce (0.0f, 3.0f);

        drawTrackedText (g, caption, captionArea, palette::skySoft.withAlpha (0.8f),
                         8.6f, 0.22f, juce::Justification::left, juce::Font::bold);

        auto bar = area.withSizeKeepingCentre (area.getWidth(), juce::jmin (16.0f, area.getHeight()));

        g.setColour (juce::Colour (0xff061429).withAlpha (0.92f));
        g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (bar.reduced (0.5f), 3.0f, 1.0f);

        auto inner = bar.reduced (2.0f);
        const float filled = inner.getWidth() * displayed;

        if (filled > 1.0f)
        {
            juce::ColourGradient gradient (palette::royalLight, inner.getX(), 0.0f,
                                           palette::skySoft, inner.getX() + inner.getWidth(), 0.0f, false);
            gradient.addColour (0.55, accent);
            g.setGradientFill (gradient);
            g.fillRoundedRectangle (inner.withWidth (filled), 2.0f);
        }

        g.setColour (juce::Colour (0xff03080f).withAlpha (0.55f));
        const float step = juce::jmax (6.0f, inner.getWidth() / 42.0f);
        for (float x = inner.getX() + step; x < inner.getRight(); x += step)
            g.drawVerticalLine ((int) x, inner.getY(), inner.getBottom());

        if (peak > 0.001f)
        {
            const auto marker = juce::Rectangle<float> (2.0f, inner.getHeight())
                                    .withCentre ({ inner.getX() + inner.getWidth() * peak, inner.getCentreY() });
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillRect (marker);
        }

        for (float db : { -60.0f, -36.0f, -12.0f, 0.0f })
        {
            const float t = (db + 60.0f) / 60.0f;
            const float x = juce::jmap (t, 0.0f, 1.0f, inner.getX(), inner.getRight());

            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.drawVerticalLine ((int) x, ticksArea.getY(), ticksArea.getY() + 3.0f);

            g.setFont (uiFont (7.8f));
            g.setColour (juce::Colours::white.withAlpha (0.36f));
            const auto labelArea = juce::Rectangle<float> (28.0f, 9.0f)
                                       .withCentre ({ juce::jlimit (inner.getX() + 10.0f, inner.getRight() - 10.0f, x),
                                                      ticksArea.getCentreY() + 2.0f });
            g.drawText (juce::String ((int) db), labelArea, juce::Justification::centred, false);
        }
    }

private:
    juce::String caption;
    juce::Colour accent;
    std::atomic<float>& leftLevel;
    std::atomic<float>& rightLevel;
    float displayed = 0.0f;
    float peak = 0.0f;
};

//==============================================================================
class VocaMaxFocusButton : public juce::Button
{
public:
    explicit VocaMaxFocusButton (const juce::String& text) : juce::Button (text)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setActive (bool shouldBeActive)
    {
        if (active != shouldBeActive)
        {
            active = shouldBeActive;
            repaint();
        }
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.0f);
        const float corner = 6.0f;

        if (active)
        {
            juce::ColourGradient gradient (palette::royalLight.withAlpha (down ? 0.8f : 0.98f),
                                           b.getX(), b.getY(),
                                           juce::Colour (0xff1a7fe0).withAlpha (down ? 0.8f : 0.98f),
                                           b.getRight(), b.getBottom(), false);
            g.setGradientFill (gradient);
            g.fillRoundedRectangle (b, corner);

            g.setColour (palette::sky.withAlpha (0.95f));
            g.drawRoundedRectangle (b.reduced (0.5f), corner, 1.3f);
            g.setColour (palette::sky.withAlpha (0.18f));
            g.drawRoundedRectangle (b.expanded (2.0f), corner + 2.0f, 3.0f);
        }
        else
        {
            g.setColour (juce::Colour (0xff07152d).withAlpha (down ? 0.95f : (highlighted ? 0.85f : 0.7f)));
            g.fillRoundedRectangle (b, corner);
            g.setColour (juce::Colours::white.withAlpha (highlighted ? 0.30f : 0.13f));
            g.drawRoundedRectangle (b.reduced (0.5f), corner, 1.0f);
        }

        drawTrackedText (g, getButtonText(), b,
                         active ? juce::Colours::white : juce::Colours::white.withAlpha (0.72f),
                         juce::jmax (8.5f, b.getHeight() * 0.30f), 0.13f,
                         juce::Justification::centred,
                         active ? juce::Font::bold : juce::Font::plain);
    }

private:
    bool active = false;
};

class VocaMaxFocusSelector : public juce::Component
{
public:
    VocaMaxFocusSelector()
    {
        const juce::StringArray labels { "NATURAL", "CLEAR", "WIDE", "DEEP" };

        for (int i = 0; i < labels.size(); ++i)
        {
            auto* button = buttons.add (new VocaMaxFocusButton (labels[i]));
            button->onClick = [this, i] { if (onChange != nullptr) onChange (i); };
            addAndMakeVisible (*button);
        }
    }

    std::function<void (int)> onChange;

    void setActiveIndex (int index)
    {
        for (int i = 0; i < buttons.size(); ++i)
            buttons[i]->setActive (i == index);
    }

    void paint (juce::Graphics& g) override
    {
        auto captionArea = getLocalBounds().toFloat().removeFromTop (16.0f);
        drawTrackedText (g, "VOCAL FOCUS MODE", captionArea, palette::skySoft.withAlpha (0.8f),
                         8.8f, 0.24f, juce::Justification::centred, juce::Font::bold);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop (18);

        const int gap = 6;
        const int buttonWidth = (area.getWidth() - gap * 3) / 4;

        for (int i = 0; i < buttons.size(); ++i)
        {
            auto b = area.removeFromLeft (buttonWidth);
            buttons[i]->setBounds (b);
            area.removeFromLeft (gap);
        }
    }

private:
    juce::OwnedArray<VocaMaxFocusButton> buttons;
};

//==============================================================================
VocaMaxSpace2AudioProcessorEditor::VocaMaxSpace2AudioProcessorEditor (VocaMaxSpace2AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    lookAndFeel = std::make_unique<VocaMaxLookAndFeel>();
    setLookAndFeel (lookAndFeel.get());

    lowKnob = std::make_unique<VocaMaxKnobPanel> ("LOW FREQUENCY", "TIGHTEN / REMOVE", "0%", "100%",
                                                  palette::sky, false, 45.0f);
    powerKnob = std::make_unique<VocaMaxKnobPanel> ("POWER", "OVERALL CONTROL", "MIN", "MAX",
                                                    palette::skySoft, true, 100.0f);
    spaceKnob = std::make_unique<VocaMaxKnobPanel> ("SPACE", "VOCAL SEPARATION", "LESS SPACE", "MORE SPACE",
                                                    palette::sky, false, 65.0f);
    highKnob = std::make_unique<VocaMaxKnobPanel> ("HIGH FREQUENCY", "BRIGHTEN / REMOVE", "0%", "100%",
                                                   palette::sky, false, 55.0f);

    for (auto* knob : { lowKnob.get(), powerKnob.get(), spaceKnob.get(), highKnob.get() })
        addAndMakeVisible (*knob);

    lowKnob->getSlider().setTooltip ("Low Frequency: tighten or remove low end (0-100%)");
    powerKnob->getSlider().setTooltip ("Power: overall control of the whole chain (0% is true bypass)");
    spaceKnob->getSlider().setTooltip ("Space: vocal separation - less space is tighter, more space is wider");
    highKnob->getSlider().setTooltip ("High Frequency: brighten or remove top end (0-100%)");

    lowAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "low", lowKnob->getSlider());
    powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "power", powerKnob->getSlider());
    spaceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "space", spaceKnob->getSlider());
    highAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "high", highKnob->getSlider());

    inputMeter = std::make_unique<VocaMaxLevelMeter> ("INPUT", palette::sky,
                                                      processor.inputLevelLeft, processor.inputLevelRight);
    outputMeter = std::make_unique<VocaMaxLevelMeter> ("OUTPUT", palette::sky,
                                                       processor.outputLevelLeft, processor.outputLevelRight);
    addAndMakeVisible (*inputMeter);
    addAndMakeVisible (*outputMeter);

    focusSelector = std::make_unique<VocaMaxFocusSelector>();
    focusSelector->onChange = [this] (int index)
    {
        if (auto* parameter = processor.apvts.getParameter ("focus"))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 ((float) index));
    };
    addAndMakeVisible (*focusSelector);

    presetBox = std::make_unique<juce::ComboBox> ("Preset");
    presetBox->addItemList (VocaMaxSpace2AudioProcessor::getFactoryPresetNames(), 1);
    presetBox->setSelectedItemIndex (processor.getCurrentProgram(), juce::dontSendNotification);
    presetBox->setTooltip ("Factory presets");
    presetBox->onChange = [this]
    {
        const int index = presetBox->getSelectedItemIndex();
        if (index >= 0 && index != processor.getCurrentProgram())
            processor.setCurrentProgram (index);
    };
    addAndMakeVisible (*presetBox);

    previousPresetButton = std::make_unique<VocaMaxIconButton> (VocaMaxIconButton::IconType::ChevronLeft,
                                                                "Previous preset");
    previousPresetButton->onClick = [this]
    {
        const int count = VocaMaxSpace2AudioProcessor::getNumFactoryPresets();
        processor.setCurrentProgram ((processor.getCurrentProgram() + count - 1) % count);
    };
    addAndMakeVisible (*previousPresetButton);

    nextPresetButton = std::make_unique<VocaMaxIconButton> (VocaMaxIconButton::IconType::ChevronRight,
                                                            "Next preset");
    nextPresetButton->onClick = [this]
    {
        const int count = VocaMaxSpace2AudioProcessor::getNumFactoryPresets();
        processor.setCurrentProgram ((processor.getCurrentProgram() + 1) % count);
    };
    addAndMakeVisible (*nextPresetButton);

    saveButton = std::make_unique<VocaMaxIconButton> (VocaMaxIconButton::IconType::Save, "Save preset to disk");
    saveButton->onClick = [this] { savePresetFile(); };
    addAndMakeVisible (*saveButton);

    settingsButton = std::make_unique<VocaMaxIconButton> (VocaMaxIconButton::IconType::Settings, "Settings and info");
    settingsButton->onClick = [this] { showSettings(); };
    addAndMakeVisible (*settingsButton);

    helpButton = std::make_unique<VocaMaxIconButton> (VocaMaxIconButton::IconType::Help, "How to use VocaMax Space 2");
    helpButton->onClick = [this] { showHelp(); };
    addAndMakeVisible (*helpButton);

    tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 800);

    focusValue = processor.apvts.getRawParameterValue ("focus");
    jassert (focusValue != nullptr);

    juce::Random random (0x564f4341);
    for (int i = 0; i < 170; ++i)
    {
        starPositions.push_back ({ random.nextFloat(), random.nextFloat() });
        starSizes.push_back (random.nextFloat() * 2.2f + 0.5f);
        starAlphas.push_back (random.nextFloat() * 0.55f + 0.12f);
    }

    setResizable (true, true);
    setResizeLimits (952, 544, 1792, 1024);
    setSize (1120, 640);

    startTimerHz (30);
}

VocaMaxSpace2AudioProcessorEditor::~VocaMaxSpace2AudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VocaMaxSpace2AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().toFloat().reduced (14.0f, 10.0f);

    logoBounds = area.removeFromTop (juce::jmax (80.0f, area.getHeight() * 0.145f));

    area.removeFromTop (4.0f);
    panelBounds = area;

    auto inner = panelBounds.reduced (panelBounds.getWidth() * 0.026f, panelBounds.getHeight() * 0.05f);
    stripBounds = inner.removeFromBottom (juce::jmax (76.0f, inner.getHeight() * 0.25f));
    inner.removeFromBottom (6.0f);
    knobRowBounds = inner;

    auto controls = logoBounds.removeFromRight (juce::jmin (440.0f, logoBounds.getWidth() * 0.46f));
    headerControlsBounds = controls;

    auto row = controls.withSizeKeepingCentre (controls.getWidth(), 30.0f);
    const float iconSize = 28.0f;

    previousPresetButton->setBounds (row.removeFromLeft (iconSize).toNearestInt());
    row.removeFromLeft (6.0f);
    helpButton->setBounds (row.removeFromRight (iconSize).toNearestInt());
    row.removeFromRight (7.0f);
    settingsButton->setBounds (row.removeFromRight (iconSize).toNearestInt());
    row.removeFromRight (7.0f);
    saveButton->setBounds (row.removeFromRight (iconSize).toNearestInt());
    row.removeFromRight (14.0f);
    nextPresetButton->setBounds (row.removeFromRight (iconSize).toNearestInt());
    row.removeFromRight (6.0f);
    presetBox->setBounds (row.toNearestInt());

    auto placeKnob = [this] (VocaMaxKnobPanel& knob, float centreFraction, float widthFraction)
    {
        const float width = knobRowBounds.getWidth() * widthFraction;
        const juce::Rectangle<float> bounds (width, knobRowBounds.getHeight());
        knob.setBounds (bounds.withCentre ({ knobRowBounds.getX() + knobRowBounds.getWidth() * centreFraction,
                                             knobRowBounds.getCentreY() }).toNearestInt());
    };

    placeKnob (*lowKnob, 0.155f, 0.20f);
    placeKnob (*powerKnob, 0.385f, 0.285f);
    placeKnob (*spaceKnob, 0.635f, 0.215f);
    placeKnob (*highKnob, 0.862f, 0.20f);

    auto strip = stripBounds;
    auto inputArea = strip.removeFromLeft (strip.getWidth() * 0.28f);
    auto outputArea = strip.removeFromRight (strip.getWidth() * 0.28f);

    inputMeter->setBounds (inputArea.toNearestInt());
    outputMeter->setBounds (outputArea.toNearestInt());
    focusSelector->setBounds (strip.reduced (10.0f, 0.0f).toNearestInt());
}

void VocaMaxSpace2AudioProcessorEditor::timerCallback()
{
    inputMeter->update();
    outputMeter->update();
    inputMeter->repaint();
    outputMeter->repaint();

    syncFromProcessor();
}

void VocaMaxSpace2AudioProcessorEditor::syncFromProcessor()
{
    if (focusValue != nullptr)
    {
        const int focus = juce::jlimit (0, 3, (int) std::lround (focusValue->load (std::memory_order_relaxed)));
        if (focus != lastFocus)
        {
            lastFocus = focus;
            focusSelector->setActiveIndex (focus);
        }
    }

    const int program = processor.getCurrentProgram();
    if (presetBox->getSelectedItemIndex() != program)
        presetBox->setSelectedItemIndex (program, juce::dontSendNotification);
}

void VocaMaxSpace2AudioProcessorEditor::savePresetFile()
{
    auto directory = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                         .getChildFile ("VocaMax Space 2 Presets");

    if (! directory.exists())
        directory.createDirectory();

    auto suggested = directory.getChildFile (processor.getProgramName (processor.getCurrentProgram()) + ".vmx2");

    fileChooser = std::make_unique<juce::FileChooser> ("Save VocaMax Space 2 preset", suggested, "*.vmx2");

    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode
                                  | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  auto file = chooser.getResult();
                                  if (file == juce::File())
                                      return;

                                  file = file.withFileExtension ("vmx2");

                                  juce::MemoryBlock state;
                                  processor.getStateInformation (state);

                                  if (! file.replaceWithData (state.getData(), state.getSize()))
                                      juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                                              "Could not save preset",
                                                                              "Check that the folder is writable.");
                              });
}

void VocaMaxSpace2AudioProcessorEditor::showSettings()
{
    const double sampleRate = processor.getSampleRate();

    juce::String message;
    message << "VocaMax Space 2  v2.0.0\n"
            << "By Mark33s Beats Lab\n\n"
            << "Host sample rate: ";
    message << (sampleRate > 0.0 ? juce::String (sampleRate, 0) + " Hz" : juce::String ("not running yet"));
    message << "\nAdded latency: 0 samples\n"
            << "Processing: 32-bit float, minimum-phase filters\n"
            << "Tuning precision: 0.1 %\n\n"
            << "Power at 0% is a true bypass of the whole chain.\n"
            << "All knob moves are smoothed to avoid zipper noise.";

    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                            "VocaMax Space 2 - Settings", message);
}

void VocaMaxSpace2AudioProcessorEditor::showHelp()
{
    juce::String message;
    message << "POWER - overall control (center knob).\n"
            << "Scales the whole chain from 0% (true bypass) to 100% (full processing).\n\n"
            << "SPACE - how much room sits between the vocal and everything else.\n"
            << "Low = tight and forward. High = wide and open. The low end stays centred.\n\n"
            << "LOW FREQUENCY (left) - tighten or remove rumble and mud.\n\n"
            << "HIGH FREQUENCY (right) - brighten or remove top end.\n\n"
            << "VOCAL FOCUS MODE - Natural, Clear, Wide and Deep add a fixed character.\n\n"
            << "Tip: double-click any knob to return it to its default.\n"
            << "Every control is automatable and saved with your project.\n\n"
            << "VST3 for Windows 10 and 11 - works in any DAW.";

    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                            "How to use VocaMax Space 2", message);
}

void VocaMaxSpace2AudioProcessorEditor::paint (juce::Graphics& g)
{
    paintBackground (g);
    paintHeader (g);
    paintPanel (g);
    paintFooter (g);
}

void VocaMaxSpace2AudioProcessorEditor::paintBackground (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient background (juce::Colour (0xff050b1a), 0.0f, 0.0f,
                                     juce::Colour (0xff020409), 0.0f, bounds.getBottom(), false);
    background.addColour (0.42, juce::Colour (0xff0a1b3d));
    g.setGradientFill (background);
    g.fillRect (bounds);

    juce::ColourGradient glow (palette::royalLight.withAlpha (0.28f), bounds.getCentreX(), bounds.getCentreY(),
                               palette::royalLight.withAlpha (0.0f), bounds.getCentreX(), bounds.getBottom(), true);
    g.setGradientFill (glow);
    g.fillRect (bounds);

    for (size_t i = 0; i < starPositions.size(); ++i)
    {
        const float x = starPositions[i].x * bounds.getWidth();
        const float y = starPositions[i].y * bounds.getHeight();
        const float size = starSizes[i];
        const float alpha = starAlphas[i];

        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.fillEllipse (x - size * 0.5f, y - size * 0.5f, size, size);

        if (size > 1.8f)
        {
            g.setColour (palette::sky.withAlpha (alpha * 0.28f));
            g.drawEllipse (x - size * 1.7f, y - size * 1.7f, size * 3.4f, size * 3.4f, 0.7f);
        }
    }

    // Distant planet, upper right.
    const juce::Point<float> planetCentre (bounds.getWidth() * 0.865f, bounds.getHeight() * 0.14f);
    const float planetRadius = bounds.getHeight() * 0.33f;

    juce::ColourGradient planet (juce::Colour (0xff164a92).withAlpha (0.55f),
                                 planetCentre.x - planetRadius * 0.35f, planetCentre.y - planetRadius * 0.35f,
                                 juce::Colour (0xff02050c).withAlpha (0.0f),
                                 planetCentre.x + planetRadius, planetCentre.y + planetRadius, true);
    g.setGradientFill (planet);
    g.fillEllipse (juce::Rectangle<float> (planetRadius * 2.0f, planetRadius * 2.0f).withCentre (planetCentre));

    g.setColour (palette::sky.withAlpha (0.18f));
    g.drawEllipse (juce::Rectangle<float> (planetRadius * 2.0f, planetRadius * 2.0f).withCentre (planetCentre), 1.0f);

    juce::Path ring;
    ring.addEllipse (-planetRadius * 1.65f, -planetRadius * 0.42f,
                      planetRadius * 3.3f, planetRadius * 0.84f);
    ring.applyTransform (juce::AffineTransform::rotation (-0.3f).translated (planetCentre.x, planetCentre.y));
    g.setColour (palette::sky.withAlpha (0.13f));
    g.strokePath (ring, juce::PathStrokeType (1.1f));
}

void VocaMaxSpace2AudioProcessorEditor::paintHeader (juce::Graphics& g)
{
    auto crownArea = juce::Rectangle<float> (27.0f, 22.0f).withPosition (logoBounds.getX() + 1.0f, logoBounds.getY() + 2.0f);
    drawCrown (g, crownArea);

    auto wordArea = juce::Rectangle<float> (logoBounds.getX(), logoBounds.getY() + 20.0f,
                                            logoBounds.getWidth(), 44.0f);
    drawGradientText (g, "VocaMax Space 2", wordArea, uiFont (37.0f, juce::Font::bold),
                      juce::Colour (0xfff2fbff), palette::sky);

    auto subtitleArea = juce::Rectangle<float> (logoBounds.getX() + 2.0f, logoBounds.getY() + 65.0f,
                                                logoBounds.getWidth(), 16.0f);
    drawTrackedText (g, "By Mark33s Beats Lab", subtitleArea, juce::Colours::white.withAlpha (0.68f),
                     11.0f, 0.10f, juce::Justification::left);

    auto tagline = juce::Rectangle<float> (headerControlsBounds.getX(),
                                           headerControlsBounds.getBottom() + 4.0f,
                                           headerControlsBounds.getWidth(), 13.0f);
    drawTrackedText (g, "VOCAL SEPARATION  |  SPACE CONTROL  |  CLARITY  |  BALANCE  |  POWER",
                     tagline, palette::sky.withAlpha (0.45f), 7.4f, 0.10f, juce::Justification::right);
}

void VocaMaxSpace2AudioProcessorEditor::paintPanel (juce::Graphics& g)
{
    const auto p = panelBounds;

    for (int i = 6; i >= 1; --i)
    {
        g.setColour (palette::sky.withAlpha (0.014f * (float) i));
        g.drawRoundedRectangle (p.expanded ((float) i * 2.0f), 20.0f + (float) i * 1.4f, 1.4f);
    }

    juce::ColourGradient fill (juce::Colour (0xe6081a38), p.getCentreX(), p.getY(),
                               juce::Colour (0xf0030712), p.getCentreX(), p.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (p, 18.0f);

    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.055f), p.getCentreX(), p.getY(),
                                 juce::Colours::white.withAlpha (0.0f), p.getCentreX(), p.getY() + p.getHeight() * 0.5f, false);
    g.setGradientFill (sheen);
    g.fillRoundedRectangle (p.reduced (1.5f), 16.5f);

    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle (p.reduced (1.5f), 16.5f);
    g.reduceClipRegion (clip);

    drawPanelWaveform (g, p, knobRowBounds.getCentreY() + p.getHeight() * 0.03f);

    const juce::Point<float> planetCentre (p.getX() + p.getWidth() * 0.60f, p.getY() + p.getHeight() * 0.31f);
    const float planetRadius = p.getHeight() * 0.17f;
    juce::ColourGradient planet (juce::Colour (0xff2a6fc4).withAlpha (0.14f),
                                 planetCentre.x - planetRadius * 0.4f, planetCentre.y - planetRadius * 0.4f,
                                 juce::Colour (0xff02060f).withAlpha (0.0f),
                                 planetCentre.x + planetRadius, planetCentre.y + planetRadius, true);
    g.setGradientFill (planet);
    g.fillEllipse (juce::Rectangle<float> (planetRadius * 2.0f, planetRadius * 2.0f).withCentre (planetCentre));

    juce::Path orbit;
    orbit.addEllipse (p.getX() + p.getWidth() * 0.42f, p.getBottom() - p.getHeight() * 0.34f,
                      p.getWidth() * 0.52f, p.getHeight() * 0.42f);
    g.setColour (palette::sky.withAlpha (0.10f));
    g.strokePath (orbit, juce::PathStrokeType (1.0f));

    g.restoreState();

    // Hairline separator above the bottom strip.
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine ((int) stripBounds.getY(), knobRowBounds.getX(), knobRowBounds.getRight());

    // Strip dividers.
    g.setColour (juce::Colours::white.withAlpha (0.055f));
    const float dividerLeft = stripBounds.getX() + stripBounds.getWidth() * 0.295f;
    const float dividerRight = stripBounds.getX() + stripBounds.getWidth() * 0.705f;
    g.drawVerticalLine ((int) dividerLeft, stripBounds.getY() + 8.0f, stripBounds.getBottom() - 6.0f);
    g.drawVerticalLine ((int) dividerRight, stripBounds.getY() + 8.0f, stripBounds.getBottom() - 6.0f);

    g.setColour (palette::sky.withAlpha (0.45f));
    g.drawRoundedRectangle (p.reduced (0.7f), 18.0f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (p.reduced (2.6f), 15.8f, 1.0f);
}

void VocaMaxSpace2AudioProcessorEditor::paintFooter (juce::Graphics& g)
{
    auto footer = getLocalBounds().toFloat().reduced (18.0f, 0.0f);
    footer = footer.removeFromBottom (22.0f);

    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine ((int) footer.getY(), footer.getX(), footer.getRight());

    auto line = footer.withTrimmedTop (3.0f);

    auto left = line.withWidth (line.getWidth() * 0.36f);
    drawTrackedText (g, "VST3  |  WINDOWS 10 & 11  |  WORKS WITH ANY DAW", left,
                     juce::Colours::white.withAlpha (0.40f), 7.8f, 0.10f,
                     juce::Justification::left, juce::Font::bold);

    auto centre = line.withTrimmedLeft (line.getWidth() * 0.36f).withTrimmedRight (line.getWidth() * 0.22f);
    drawTrackedText (g, "VOCAL SEPARATION  |  SPACE CONTROL  |  CLARITY  |  BALANCE  |  POWER", centre,
                     palette::sky.withAlpha (0.55f), 7.8f, 0.10f, juce::Justification::centred);

    auto right = line.removeFromRight (line.getWidth() * 0.22f);
    drawTrackedText (g, "MARK33S BEATS LAB", right, juce::Colours::white.withAlpha (0.36f),
                     7.8f, 0.14f, juce::Justification::right, juce::Font::bold);
}
