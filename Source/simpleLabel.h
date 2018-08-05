/*
  ==============================================================================

    simpleLabel.h
    Created: 8 Aug 2017 3:32:22pm
    Author:  Daniel Rudrich

  ==============================================================================
*/

#pragma once


//==============================================================================
/*
*/
class simpleLabel    : public Component
{
public:
    simpleLabel()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.

    }
    ~simpleLabel()
    {
    }

    void setText(String newText) {
        text = newText;
        repaint();
    }
    void setText(String newText, bool newBold) {
        text = newText;
        isBold = newBold;
        repaint();
    }
    void setText(String newText, bool newBold, Justification newJustification) {
        text = newText;
        isBold = newBold;
        justification = newJustification;
        repaint();
    }
    
    void setJustification(Justification newJustification) {
        justification = newJustification;
        repaint();
    }
    
    
    void paint (Graphics& g) override
    {
        Rectangle<int> bounds = getLocalBounds();
        paintSimpleLabel(g, bounds, text, isBold,justification);
    }
    
    virtual void paintSimpleLabel(Graphics& g, Rectangle<int> bounds, String text, bool isBold, Justification justification)
    {
        g.setColour (Colours::white);
        g.setFont (bounds.getHeight());
        g.setFont(getLookAndFeel().getTypefaceForFont(Font(bounds.getHeight())));
        g.drawText (text, bounds,
                    justification, true);
    }

    void resized() override
    {
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (simpleLabel)
    String text = "";
    bool isBold = false;
    Justification justification = Justification::centred;
};


//==============================================================================
/*
 */
class tripleLabel    : public Component
{
public:
    tripleLabel()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        
    }
    
    ~tripleLabel()
    {
    }
    
    void setText(String newLeftText, String newMiddleText, String newRightText, bool newLeftBold, bool newMiddleBold, bool newRightBold) {
        leftText = newLeftText;
        middleText = newMiddleText;
        rightText = newRightText;
        leftBold = newLeftBold;
        middleBold = newMiddleBold;
        rightBold = newRightBold;
        
        repaint();
    }
    
    void paint (Graphics& g) override
    {
        Rectangle<int> bounds = getLocalBounds();
        paintTripleLabel(g, bounds, leftText, middleText, rightText, leftBold, middleBold, rightBold);
    }
    
    virtual void paintTripleLabel(Graphics& g, Rectangle<int> bounds, String leftText, String middleText, String rightText, bool leftBold, bool middleBold, bool lrightBold)
    {
        g.setColour (Colours::white);
        Font tempFont;
        tempFont.setHeight(bounds.getHeight());
        int height = bounds.getHeight();
        
        tempFont.setStyleFlags(leftBold ? 1 : 0);
        g.setFont(getLookAndFeel().getTypefaceForFont(tempFont));
        g.setFont(height);
        g.drawText (leftText, bounds, Justification::left, true);
        
        tempFont.setStyleFlags(middleBold ? 1 : 0);
        g.setFont(getLookAndFeel().getTypefaceForFont(tempFont));
        g.setFont(height + (middleBold ? 2 : 0));
        g.drawText (middleText, bounds, Justification::centred, true);
        
        tempFont.setStyleFlags(rightBold ? 1 : 0);
        g.setFont(getLookAndFeel().getTypefaceForFont(tempFont));
        g.setFont(height);
        g.drawText (rightText, bounds, Justification::right, true);
    }
    
    
    void resized() override
    {
    }
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (tripleLabel)
    String leftText = "";
    String middleText = "";
    String rightText = "";
    bool leftBold, middleBold, rightBold;

};

