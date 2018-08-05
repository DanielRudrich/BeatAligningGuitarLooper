/*
 ==============================================================================
 
 SliderLaF.h
 Created: 25 Apr 2017 9:17:33pm
 Author:  Daniel Rudrich
 
 ==============================================================================
 */

#ifndef SLIDERLAF_H_INCLUDED
#define SLIDERLAF_H_INCLUDED


class LaF : public LookAndFeel_V4
{
public:
    Colour ClBackground = Colour(0xFF2D2D2D);
    Colour ClFace = Colour(0xFFD8D8D8);
    Colour ClFaceShadow = Colour(0XFF272727);
    Colour ClFaceShadowOutline = Colour(0xFF212121);
    Colour ClFaceShadowOutlineActive = Colour(0xFF7C7C7C);
    Colour ClWidgetColours[4];
    Colour ClRotSliderArrow = Colour(0xFF4A4A4A);
    Colour ClRotSliderArrowShadow = Colour(0x445D5D5D);
    Colour ClSliderFace = Colour(0xFF191919);
    Colour ClText = Colour(0xFFFFFFFF);
    Colour ClTextTextboxbg = Colour(0xFF000000);
    Colour ClSeperator = Colour(0xFF979797);
    
    
    Typeface::Ptr robotoLight, robotoRegular, robotoMedium, robotoBold;
    
    float sliderThumbDiameter = 14.0f;
    float sliderBarSize = 8.0f;
    
    //    Colour ClBackground = Colour(0xFF2D2D2D);
    //    Colour ClBackground = Colour(0xFF2D2D2D);
    LaF()
    {
        robotoLight = Typeface::createSystemTypefaceFor(BinaryData::RobotoLight_ttf, BinaryData::RobotoLight_ttfSize); //TODO: free this data
        robotoMedium = Typeface::createSystemTypefaceFor(BinaryData::RobotoMedium_ttf, BinaryData::RobotoMedium_ttfSize);
        robotoBold = Typeface::createSystemTypefaceFor(BinaryData::RobotoBold_ttf, BinaryData::RobotoBold_ttfSize);
        robotoRegular = Typeface::createSystemTypefaceFor(BinaryData::RobotoRegular_ttf, BinaryData::RobotoRegular_ttfSize); //
        
        
        ClWidgetColours[0] = Colour(0xFF00CAFF);
        ClWidgetColours[1] = Colour(0xFF4FFF00);
        ClWidgetColours[2] = Colour(0xFFFF9F00);
        ClWidgetColours[3] = Colour(0xFFD0011B);
        setColour (Slider::rotarySliderFillColourId, Colours::black);
        setColour (Slider::thumbColourId, Colour (0xCCFFFFFF));
        
    }
    Typeface::Ptr getTypefaceForFont (const Font& f) override
    {
        switch (f.getStyleFlags()) {
            case 1: return robotoBold;
            default: return robotoRegular;
        }
    }
    Font getLabelFont (Label& label) override
    {
        //return label.getFont();
        return Font(robotoMedium);
    }
    Font getPopupMenuFont() override
    {
        return Font (14.0f);
    }
    Slider::SliderLayout getSliderLayout (Slider& slider) override
    {
        
        // 1. compute the actually visible textBox size from the slider textBox size and some additional constraints
        
        int minXSpace = 0;
        int minYSpace = 0;
        
        Slider::TextEntryBoxPosition textBoxPos = slider.getTextBoxPosition();
        
        if (textBoxPos == Slider::TextBoxLeft || textBoxPos == Slider::TextBoxRight)
            minXSpace = 30;
        else
            minYSpace = 15;
        
        Rectangle<int> localBounds = slider.getLocalBounds();
        
        const int textBoxWidth = jmax (0, jmin (slider.getTextBoxWidth(),  localBounds.getWidth() - minXSpace));
        const int textBoxHeight = jmax (0, jmin (slider.getTextBoxHeight(), localBounds.getHeight() - minYSpace));
        
        Slider::SliderLayout layout;
        
        // 2. set the textBox bounds
        
        if (textBoxPos != Slider::NoTextBox)
        {
            if (slider.isBar())
            {
                layout.textBoxBounds = localBounds;
            }
            else
            {
                layout.textBoxBounds.setWidth (textBoxWidth);
                layout.textBoxBounds.setHeight (textBoxHeight);
                
                if (textBoxPos == Slider::TextBoxLeft)           layout.textBoxBounds.setX (0);
                    else if (textBoxPos == Slider::TextBoxRight)     layout.textBoxBounds.setX (localBounds.getWidth() - textBoxWidth);
                        else /* above or below -> centre horizontally */ layout.textBoxBounds.setX ((localBounds.getWidth() - textBoxWidth) / 2);
                            
                            if (textBoxPos == Slider::TextBoxAbove)          layout.textBoxBounds.setY (0);
                                else if (textBoxPos == Slider::TextBoxBelow)     layout.textBoxBounds.setY (localBounds.getHeight() - textBoxHeight);
                                    else /* left or right -> centre vertically */    layout.textBoxBounds.setY ((localBounds.getHeight() - textBoxHeight) / 2);
                                        }
        }
        
        // 3. set the slider bounds
        
        layout.sliderBounds = localBounds;
        
        if (slider.isBar())
        {
            layout.sliderBounds.reduce (1, 1);   // bar border
        }
        else
        {
            if (textBoxPos == Slider::TextBoxLeft)       layout.sliderBounds.removeFromLeft (textBoxWidth);
                else if (textBoxPos == Slider::TextBoxRight) layout.sliderBounds.removeFromRight (textBoxWidth);
                    else if (textBoxPos == Slider::TextBoxAbove) layout.sliderBounds.removeFromTop (textBoxHeight);
                        else if (textBoxPos == Slider::TextBoxBelow) layout.sliderBounds.removeFromBottom (textBoxHeight);
                            
                            const int thumbIndent = getSliderThumbRadius (slider);
                            
                            if (slider.isHorizontal())    layout.sliderBounds.reduce (thumbIndent, 0);
                                else if (slider.isVertical()) layout.sliderBounds.reduce (0, thumbIndent);
                                    }
        
        return layout;
        
    }
    
    
    void drawLabel (Graphics& g, Label& label) override
    {
        float alpha = label.isEnabled() ? 1.0f : 0.4f;
        g.fillAll (label.findColour (Label::backgroundColourId));
        Rectangle<int> bounds = label.getLocalBounds();
        float x = (float) bounds.getX();
        float y = (float) bounds.getY();
        float w = (float) bounds.getWidth();
        float h = (float) bounds.getHeight();
        Path p;
        p.addRoundedRectangle(x, y, w, h, h/2.0f);
        g.setColour (ClTextTextboxbg.withMultipliedAlpha(alpha));
        g.fillPath (p);
        
        if (! label.isBeingEdited())
        {
            const float alpha = label.isEnabled() ? 1.0f : 0.5f;
            const Font font (robotoLight);
            
            //g.setColour (ClText.withMultipliedAlpha (alpha));
            g.setColour (ClText.withMultipliedAlpha(alpha));
            g.setFont (robotoMedium);
            g.setFont (h-2.0f);
            
            Rectangle<int> textArea (label.getBorderSize().subtractedFrom (label.getLocalBounds()));
            
            g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                              jmax (1, (int) (textArea.getHeight() / font.getHeight())),
                              label.getMinimumHorizontalScale());
            
            g.setColour (label.findColour (Label::outlineColourId).withMultipliedAlpha (alpha));
        }
        else if (label.isEnabled())
        {
            g.setColour (label.findColour (Label::outlineColourId));
        }
        //g.drawRect (label.getLocalBounds());
        //g.drawRoundedRectangle (0,0,80,10,7.f,2);
        
    }
    
    void fillTextEditorBackground (Graphics& g, int width, int height, TextEditor& textEditor) override
    {
                if (dynamic_cast<AlertWindow*> (textEditor.getParentComponent()) != nullptr)
                {
                    g.setColour (textEditor.findColour (TextEditor::backgroundColourId));
                    g.fillRect (0, 0, width, height);
        
                    g.setColour (textEditor.findColour (TextEditor::outlineColourId));
                    g.drawHorizontalLine (height - 1, 0.0f, static_cast<float> (width));
                }
                else
                {
                    Path p;
                    p.addRoundedRectangle(0, 0, width, height, height/2.0f);
                    g.setColour (ClTextTextboxbg);
                    g.fillPath (p);
                }
    }
    
    
    void drawTextEditorOutline (Graphics& g, int width, int height, TextEditor& textEditor) override
    {
        if (dynamic_cast<AlertWindow*> (textEditor.getParentComponent()) == nullptr)
        {
            if (textEditor.isEnabled())
            {
                if (textEditor.hasKeyboardFocus (true) && ! textEditor.isReadOnly())
                {
                    g.setColour (Colours::white.withMultipliedAlpha(0.8f));
                    g.drawRoundedRectangle (0.5, 0.5, width-1, height-1, (height-1)/2.0f, 0.8);
                    
                }
                else
                {
                    g.setColour (Colours::white.withMultipliedAlpha(0.8f));
                    g.drawRoundedRectangle (0, 0, width, height, height/2.0f, 0);
                }
            }
        }
    }
    
    
    void drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const Slider::SliderStyle style, Slider& slider) override
    {
        //g.fillAll (slider.findColour (Slider::backgroundColourId));
        
        //Label* l = createSliderTextBox(slider);
        //l->showEditor();
        
        if (style == Slider::LinearBar || style == Slider::LinearBarVertical)
        {
            const float fx = (float) x, fy = (float) y, fw = (float) width, fh = (float) height;
            
            Path p;
            
            if (style == Slider::LinearBarVertical)
                p.addRectangle (fx, sliderPos, fw, 1.0f + fh - sliderPos);
                else
                    p.addRectangle (fx, fy, sliderPos - fx, fh);
                    
                    
                    Colour baseColour (slider.findColour (Slider::rotarySliderFillColourId)
                                       .withMultipliedSaturation (slider.isEnabled() ? 1.0f : 0.5f)
                                       .withMultipliedAlpha (1.0f));
                    
                    g.setColour (baseColour);
                    g.fillPath (p);
                    
                    const float lineThickness = jmin (15.0f, jmin (width, height) * 0.45f) * 0.1f;
            g.drawRect (slider.getLocalBounds().toFloat(), lineThickness);
        }
        else
        {
            drawLinearSliderBackground (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
            drawLinearSliderThumb (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }
    
    void drawLinearSliderBackground (Graphics& g, int x, int y, int width, int height,
                                     float sliderPos,
                                     float minSliderPos,
                                     float maxSliderPos,
                                     const Slider::SliderStyle style, Slider& slider) override
    {
        const float sliderRadius = 8.f; //getSliderThumbRadius (slider) - 5.0f;
        Path slbg;
        Path clbar;
        
        Colour statusColour = slider.findColour(Slider::rotarySliderOutlineColourId).withMultipliedAlpha (0.3f);
        
        
        const float min = slider.getMinimum();
        const float max = slider.getMaximum();
        const float zeroPos = -min/(max-min);
        bool isTwoValue = (style == Slider::SliderStyle::TwoValueVertical || style == Slider::SliderStyle::TwoValueHorizontal);
        
        if (slider.isHorizontal())
        {
            const float iy = y + height * 0.5f - sliderRadius * 0.5f;
            Rectangle<float> r (x - sliderRadius * 0.5f, iy, width + sliderRadius, sliderRadius);
            slbg.addRoundedRectangle (r,sliderRadius/2.0,sliderRadius/2.0);
            
            if (isTwoValue)
            {
                clbar.addRoundedRectangle(Rectangle<float>(Point<float>(minSliderPos, iy), Point<float>(maxSliderPos, iy+sliderRadius)),sliderRadius/2.0,sliderRadius/2.0);
            }
            else
            {
            clbar.addRoundedRectangle(Rectangle<float>(Point<float>(x+width*zeroPos, iy), Point<float>(sliderPos, iy+sliderRadius)),sliderRadius/2.0,sliderRadius/2.0);
            }
        }
        else
        {
            const float ix = x + width * 0.5f - sliderRadius * 0.5f;
            Rectangle<float> r (ix, y - sliderRadius * 0.5f, sliderRadius, height + sliderRadius);
            slbg.addRoundedRectangle (r,sliderRadius/2.0,sliderRadius/2.0);
            clbar.addRoundedRectangle(Rectangle<float>(Point<float>(ix+1.0f,y+height*zeroPos), Point<float>(ix-1.0f+sliderRadius,sliderPos)),sliderRadius/2.0,sliderRadius/2.0);
        }
        
        
        g.setColour(ClSliderFace);
        g.fillPath(slbg);
        g.setColour(statusColour);
        g.fillPath(clbar);
        g.setColour(ClFaceShadowOutline);
        
        g.strokePath(slbg, PathStrokeType(1.f));

        
    }
    
    
    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, Slider& slider) override
    {
        drawRotarySliderDual (g, x, y, width,height, sliderPos,
                              rotaryStartAngle, rotaryEndAngle, slider, false);
    }
    
    
    void drawRotarySliderDual (Graphics& g, int x, int y, int width, int height, float sliderPos,
                               float rotaryStartAngle, float rotaryEndAngle, Slider& slider, bool isDual)
    {
        bool isEnabled = slider.isEnabled();
        const float alpha = isEnabled ? 1.0f : 0.4f;
        const float radius = jmin (width / 2, height / 2);
        const float centreX = x + width * 0.5f;
        const float centreY = y + height * 0.5f;
        const float rx = centreX - radius;
        const float ry = centreY - radius;
        const float rw = radius * 2.0f;
        
        const float min = slider.getMinimum();
        const float max = slider.getMaximum();
        const float zeroPos = -min/(max-min);
        const float zeroAngle =rotaryStartAngle + zeroPos * (rotaryEndAngle - rotaryStartAngle);
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const float negAngle = rotaryStartAngle + (2*zeroPos-sliderPos) * (rotaryEndAngle - rotaryStartAngle);
        
        
        const float bedThickness = 2.0f;
        const float bedOutline = 1.4f;
        const float statusOutline = 1.6f;
        const float extraMargin = 1.0f;
        
        const float pointerThickness = 1.2f;
        const float pointerLength = (radius - extraMargin - statusOutline - bedOutline - bedThickness - 1.0f) * 0.8f;
        
        Path p,q,a;
        Rectangle<float> r = Rectangle<float>(rx, ry, rw, rw);
        
        const bool isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();
        
        const Colour statusColour = slider.findColour(Slider::rotarySliderOutlineColourId);
        //status ring
        g.setColour (statusColour.withMultipliedAlpha(alpha));
        
        
        a.addCentredArc(centreX,centreY,radius-extraMargin,radius-extraMargin,0.0f,zeroAngle,angle,true);
        if (isDual) a.addCentredArc(centreX,centreY,radius-extraMargin,radius-extraMargin,0.0f,negAngle,zeroAngle,true);
        g.strokePath(a, PathStrokeType(statusOutline));
        
        //bed ellipse
        g.setColour (ClFaceShadow);
        g.fillEllipse (r.reduced(extraMargin+statusOutline));
        
        //(isMouseOver)?g.setColour(ClFaceShadowOutlineActive) : g.setColour (ClFaceShadowOutline);
        (isMouseOver)?g.setColour(statusColour.withMultipliedAlpha(0.4f)) : g.setColour (ClFaceShadowOutline);
        g.drawEllipse (r.reduced(extraMargin+statusOutline), bedOutline);
        
        
        //knob
        g.setColour (ClFace.withMultipliedAlpha(alpha));
        g.fillEllipse (r.reduced(extraMargin+statusOutline+bedOutline+bedThickness));
        g.setColour (statusColour.withMultipliedAlpha(alpha));
        g.drawEllipse (r.reduced(extraMargin+statusOutline+bedOutline+bedThickness), statusOutline);
        
        g.setColour (ClRotSliderArrowShadow.withMultipliedAlpha(alpha));
        g.drawEllipse (r.reduced(extraMargin+statusOutline+bedOutline+bedThickness+1.0f), 1.0f  );
        
        q.addRectangle (pointerThickness * 0.3f, -radius+6.0f, pointerThickness, pointerLength);
        q.applyTransform (AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (ClRotSliderArrowShadow.withMultipliedAlpha(alpha));
        g.fillPath (q);
        
        p.addRectangle (-pointerThickness * 0.5f, -radius+6.0f, pointerThickness, pointerLength);
        p.applyTransform (AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (ClRotSliderArrow.withMultipliedAlpha(alpha));
        g.fillPath (p);
        
    }
    
    
    void drawLinearSliderThumb (Graphics& g, int x, int y, int width, int height,
                                float sliderPos, float minSliderPos, float maxSliderPos,
                                const Slider::SliderStyle style, Slider& slider) override
    {
        const float sliderRadius = (float) (getSliderThumbRadius (slider) - 2);
        
        //bool isDownOrDragging = slider.isEnabled() && (slider.isMouseOverOrDragging() || slider.isMouseButtonDown());
        //Colour knobColour (slider.findColour (Slider::thumbColourId).withMultipliedSaturation ((slider.hasKeyboardFocus (false) || isDownOrDragging) ? 1.3f : 0.9f)
                          // .withMultipliedAlpha (slider.isEnabled() ? 1.0f : 0.7f));
        Colour knobColour = slider.findColour(Slider::rotarySliderOutlineColourId).withMultipliedAlpha (slider.isEnabled() ? 1.0f : 0.7f);
        const float outlineThickness = slider.isEnabled() ? 1.9f : 0.3f;
        
        if (style == Slider::LinearHorizontal || style == Slider::LinearVertical)
        {
            float kx, ky;
            
            if (style == Slider::LinearVertical)
            {
                kx = x + width * 0.5f;
                ky = sliderPos;
            }
            else
            {
                kx = sliderPos;
                ky = y + height * 0.5f;
            }
            

            
            drawRoundThumb (g,
                            kx - sliderRadius,
                            ky - sliderRadius,
                            sliderRadius * 2.0f,
                            knobColour, outlineThickness);
        }
        else if (style == Slider::TwoValueVertical)
        {
            drawRoundThumb (g,
                            jmax (0.0f, x + width * 0.5f - sliderRadius * 2.0f),
                            minSliderPos - sliderRadius,
                            sliderRadius * 2.0f,
                            knobColour, outlineThickness);
            
            drawRoundThumb (g,
                            jmax (0.0f, x + width * 0.5f - sliderRadius * 2.0f),
                            maxSliderPos - sliderRadius,
                            sliderRadius * 2.0f,
                            knobColour, outlineThickness);
            }
        else if (style == Slider::TwoValueHorizontal )
        {
            drawRoundThumb (g,
                            minSliderPos-sliderRadius,
                            jmax (0.0f, y + height * 0.5f - sliderRadius * 2.0f),
                            sliderRadius * 2.0f,
                            knobColour, outlineThickness);
            
            drawRoundThumb (g,
                            maxSliderPos-sliderRadius,
                            jmax (0.0f, y + height * 0.5f - sliderRadius * 2.0f),
                            sliderRadius * 2.0f,
                            knobColour, outlineThickness);
            

            }
        else
        {
            // Just call the base class for the demo
            LookAndFeel_V2::drawLinearSliderThumb (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }
    void drawRoundThumb (Graphics& g, const float x, const float y,
                         const float diameter, const Colour& colour, float outlineThickness)
    {
        //        const Rectangle<float> a (x, y, diameter, diameter);
        const float halfThickness = outlineThickness * 0.5f;
        Path p;
        p.addEllipse (x + halfThickness, y + halfThickness, sliderThumbDiameter - outlineThickness, sliderThumbDiameter - outlineThickness);
        
        g.setColour (ClFace);
        g.fillPath (p);
        
        g.setColour (colour);
        g.strokePath (p, PathStrokeType (outlineThickness));
        g.setColour (ClRotSliderArrowShadow);
        g.drawEllipse (x + halfThickness+1.0f, y + halfThickness+1.0f, sliderThumbDiameter - outlineThickness-1.0f, sliderThumbDiameter - outlineThickness-1.0f, 1.4f);
    }
    
    
    Button* createSliderButton (Slider&, const bool isIncrement) override
    {
        return new TextButton (isIncrement ? "+" : "-", String());
        //return new ArrowButton (String(),isIncrement ? 0.75 : 0.25f,Colours::white);
    }

    
    
    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        Colour baseColour (Colours::black.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                           .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
        
        if (isButtonDown || isMouseOverButton) baseColour = baseColour.contrasting (isButtonDown ? 0.3f : 0.1f);
            
        const bool flatOnLeft   = button.isConnectedOnLeft();
        const bool flatOnRight  = button.isConnectedOnRight();
        const bool flatOnTop    = button.isConnectedOnTop();
        const bool flatOnBottom = button.isConnectedOnBottom();
            
        const float width  = button.getWidth();
        const float height = button.getHeight() ;
        
        if (width > 0 && height > 0)
        {
            const float cornerSize = jmin (15.0f, jmin (width, height) * 0.45f);

            
            Path outline;
            outline.addRoundedRectangle (0.0f, 0.0f, width, height,
                                         cornerSize, cornerSize,
                                         ! (flatOnLeft  || flatOnTop),
                                         ! (flatOnRight || flatOnTop),
                                         ! (flatOnLeft  || flatOnBottom),
                                         ! (flatOnRight || flatOnBottom));
            
            
            g.setColour (baseColour);
            g.fillPath (outline);
            

        }
    }
    
    void drawToggleButton (Graphics& g, ToggleButton& button,
                                           bool isMouseOverButton, bool isButtonDown) override
    {
        if (button.getButtonText() == "ON/OFF")
        {
            Colour baseColour (Colours::black.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                               .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));

                
            const float width  = button.getWidth();
            const float height = button.getHeight() ;
            bool isOn = button.getToggleState();
            const float cornerSize = jmin (15.0f, jmin (width, height) * 0.45f);
                    
                    
            Path outline;
            outline.addRoundedRectangle (0.5f, 0.5f, width-1, height-1,
                                                 cornerSize, cornerSize);
                    
                    
            g.setColour (baseColour);
            g.fillPath (outline);
            if (isMouseOverButton)
            {
                g.setColour (button.findColour(ToggleButton::tickColourId).withMultipliedAlpha(isButtonDown ? 0.8f : 0.4f));
                g.strokePath(outline,PathStrokeType(isButtonDown ? 1.0f : 0.8f));
            }
            g.setFont(robotoMedium);
            g.setFont(height-1);
            g.setColour (isOn ? button.findColour(ToggleButton::tickColourId) : Colours::white);
            g.drawText(isOn ? "ON" : "OFF" , 0, 0, width, height, Justification::centred);
            
        }
        
        else
        {
            const auto fontSize = jmin (15.0f, button.getHeight() * 0.75f);
            const auto tickWidth = fontSize * 1.1f;
            
            drawTickBox (g, button, 4.0f, (button.getHeight() - tickWidth) * 0.5f,
                         tickWidth, tickWidth,
                         button.getToggleState(),
                         button.isEnabled(),
                         isMouseOverButton,
                         isButtonDown);
            
            g.setColour (button.findColour (ToggleButton::textColourId));
            g.setFont (fontSize);
            
            if (! button.isEnabled())
                g.setOpacity (0.5f);
                
                g.drawFittedText (button.getButtonText(),
                                  button.getLocalBounds().withTrimmedLeft (roundToInt (tickWidth) + 10)
                                  .withTrimmedRight (2),
                                  Justification::centredLeft, 10);
                }
    }
    
    void drawTickBox (Graphics& g, Component& component,
                      float x, float y, float w, float h,
                      bool ticked,
                      bool isEnabled,
                      bool isMouseOverButton,
                      bool isButtonDown) override
    {
        const float boxSize = w * 0.7f;
        
        bool isDownOrDragging = component.isEnabled() && (component.isMouseOverOrDragging() || component.isMouseButtonDown());
        const Colour colour (component.findColour (TextButton::buttonColourId).withMultipliedSaturation ((component.hasKeyboardFocus (false) || isDownOrDragging) ? 1.3f : 0.9f)
                             .withMultipliedAlpha (component.isEnabled() ? 1.0f : 0.7f));
        
        drawRoundThumb (g, x, y + (h - boxSize) * 0.5f, boxSize, colour,
                        isEnabled ? ((isButtonDown || isMouseOverButton) ? 1.1f : 0.5f) : 0.3f);
        
        if (ticked)
        {
            const Path tick (LookAndFeel_V2::getTickShape (6.0f));
            g.setColour (isEnabled ? findColour (TextButton::buttonOnColourId) : Colours::grey);
            
            const float scale = 9.0f;
            const AffineTransform trans (AffineTransform::scale (w / scale, h / scale)
                                         .translated (x - 2.5f, y + 1.0f));
            g.fillPath (tick, trans);
        }
    }
    
    void drawGroupComponentOutline (Graphics& g, int width, int height,
                                    const String& text, const Justification& position,
                                    GroupComponent& group) override
    {
        Rectangle<int> r(6,0,width-6,15);
        g.setColour(ClText);
        g.setFont(robotoMedium);
        g.setFont(18.0f);
        g.drawFittedText (text, r, position,1,0.f);
        
        g.setColour(ClSeperator);
        g.drawLine(0, 18, width, 18 ,.8f);
    }
    void positionComboBoxText (ComboBox& box, Label& label) override
    {
        label.setBounds (0, 0,
                         box.getWidth() - box.getHeight(),
                         box.getHeight());
        
        label.setFont (getLabelFont(label));
    }
    
    void drawComboBox (Graphics& g, int width, int height, bool isButtonDown,
                            int buttonX, int buttonY, int buttonW, int buttonH,
                            ComboBox& box) override
    {
        //const auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
//        const Rectangle<int> boxBounds (0, 0, width, height);
//        
//        g.setColour (box.findColour (ComboBox::backgroundColourId));
//        g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);
//        
//        g.setColour (box.findColour (ComboBox::outlineColourId));
//        g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);
        
        Rectangle<int> buttonArea (buttonX, buttonY, buttonW, buttonH);
        Path path;
        path.startNewSubPath (buttonX + 3.0f, buttonArea.getCentreY() - 2.0f);
        path.lineTo (buttonArea.getCentreX(), buttonArea.getCentreY() + 3.0f);
        path.lineTo (buttonArea.getRight() - 3.0f, buttonArea.getCentreY() - 2.0f);
        
        g.setColour (Colours::white.withAlpha ((box.isEnabled() ? 0.9f : 0.2f)));
        g.strokePath (path, PathStrokeType (2.0f));
    }
    
};



#endif  // SLIDERLAF_H_INCLUDED
