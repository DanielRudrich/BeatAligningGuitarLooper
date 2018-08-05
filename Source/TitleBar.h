
#pragma once





class  TitleBar :  public Component
{
public:
    TitleBar() : Component() {

    };
    ~TitleBar() {};
    

    
    void setTitle (String newBoldText, String newRegularText) {
        boldText = newBoldText;
        regularText = newRegularText;
    }
    
    void setFont (Typeface::Ptr newBoldFont, Typeface::Ptr newRegularFont) {
        boldFont = newBoldFont;
        regularFont = newRegularFont;
    }
    
    void resized () override
    {
        Rectangle<int> bounds = getLocalBounds();
        bounds = bounds.removeFromRight(110);
        bounds.reduce(0,15);

    }
    
    void paint (Graphics& g) override
    {
        Rectangle<int> bounds = getLocalBounds();
        const float centreX = bounds.getX() + bounds.getWidth()*0.5;
        const float centreY = bounds.getY() + bounds.getHeight()*0.5;
        const float boldHeight = 35.f;
        const float regularHeight = 35.f;
        
        boldFont.setHeight(boldHeight);
        regularFont.setHeight(regularHeight);
        
        const float boldWidth = boldFont.getStringWidth(boldText);
        const float regularWidth = regularFont.getStringWidth(regularText);
        
        Rectangle<float> textArea (0, 0, boldWidth+regularWidth,jmax(boldHeight,regularHeight));
        textArea.setCentre(centreX,centreY);
        
        
        
        //g.fillRect(textArea);
        
        g.setColour(Colours::white);
        g.setFont(boldFont);
        g.drawFittedText(boldText, textArea.removeFromLeft(boldWidth).toNearestInt(), Justification::bottom, 1);
        g.setFont(regularFont);
        g.drawFittedText(regularText, textArea.toNearestInt(), Justification::bottom, 1);
        
        g.setColour((Colours::white).withMultipliedAlpha(0.5));
        g.drawLine(bounds.getX(),bounds.getY()+bounds.getHeight()-4, bounds.getX()+bounds.getWidth(), bounds.getY()+bounds.getHeight()-4);

    };
private:

    Font boldFont = Font(25.f);
    Font regularFont = Font(25.f);
    juce::String boldText = "Bold";
    juce::String regularText = "Regular";
    
};




