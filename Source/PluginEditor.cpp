/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    auto enabled = slider.isEnabled();

    g.setColour(enabled ? Colour(128, 0, 128) : Colours::darkgrey);
    g.fillEllipse(bounds);

    g.setColour(enabled ? Colour(216, 191, 216) : Colours::grey);
    g.drawEllipse(bounds, 1.f);

    if (auto* rswl = dynamic_cast<SliderRotatorioConEtiquetas*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());

        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics& g,
    juce::ToggleButton& toggleButton,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    using namespace juce;

    if (auto* pb = dynamic_cast<BotonEncendido*>(&toggleButton))
    {
        Path powerButton;

        auto bounds = toggleButton.getLocalBounds();

        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

        float ang = 30.f; //30.f;

        size -= 6;

        powerButton.addCentredArc(r.getCentreX(),
            r.getCentreY(),
            size * 0.5,
            size * 0.5,
            0.f,
            degreesToRadians(ang),
            degreesToRadians(360.f - ang),
            true);

        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());

        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

        auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(73u, 243u, 242u);

        g.setColour(color);
        g.strokePath(powerButton, pst);
        g.drawEllipse(r, 2);
    }
    else if (auto* botonAnalizador = dynamic_cast<BotonAnalizador*>(&toggleButton))
    {
        auto color = !toggleButton.getToggleState() ? Colours::dimgrey : Colour(73u, 243u, 242u);

        g.setColour(color);

        auto bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);

        g.strokePath(botonAnalizador->randomPath, PathStrokeType(1.f));
    }
}
//==============================================================================
void SliderRotatorioConEtiquetas::paint(juce::Graphics& g)
{
    using namespace juce;

    auto anguloDeInicio = degreesToRadians(180.f + 45.f);
    auto anguloDeFin = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto rango = getRange();

    auto sliderBounds = getSliderBounds();

    getLookAndFeel().drawRotarySlider(g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        jmap(getValue(), rango.getStart(), rango.getEnd(), 0.0, 1.0),
        anguloDeInicio,
        anguloDeFin,
        *this);

    auto centro = sliderBounds.toFloat().getCentre();
    auto radio = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(0, 255, 255));
    g.setFont(getTextHeight());

    auto numChoices = etiquetas.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = etiquetas[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        auto ang = jmap(pos, 0.f, 1.f, anguloDeInicio, anguloDeFin);

        auto c = centro.getPointOnCircumference(radio + getTextHeight() * 0.5f + 1, ang);

        Rectangle<float> r;
        auto str = etiquetas[i].etiqueta;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }

}

juce::Rectangle<int> SliderRotatorioConEtiquetas::getSliderBounds() const
{
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;

}

juce::String SliderRotatorioConEtiquetas::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    bool addK = false;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();

        if (val > 999.f)
        {
            val /= 1000.f; //1001 / 1000 = 1.001
            addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse; //Esto no debería ocurrir
    }

    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
            str << "k";

        str << suffix;
    }

    return str;
}
//==============================================================================
ComponenteAnalizador::ComponenteAnalizador(MonitorDeEspectroDeSeñalAudioProcessor& p) :
    audioProcessor(p),
    productorOndaIzq(audioProcessor.canalIzqFIFO),
    productorOndaDer(audioProcessor.canalDerFIFO)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    updateChain();

    startTimerHz(60);
}

ComponenteAnalizador::~ComponenteAnalizador()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ComponenteAnalizador::actualizaSeñal()
{
    using namespace juce;
    auto responseArea = getAnalysisArea();

    auto w = responseArea.getWidth();

    auto& parteBaja = cadena.get<PosicionCadenas::Bajo>();
    auto& pico = cadena.get<PosicionCadenas::Pico>();
    auto& parteAlta = cadena.get<PosicionCadenas::Alto>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);

    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (!cadena.isBypassed<PosicionCadenas::Pico>())
            mag *= pico.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!cadena.isBypassed<PosicionCadenas::Bajo>())
        {
            if (!parteBaja.isBypassed<0>())
                mag *= parteBaja.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!parteBaja.isBypassed<1>())
                mag *= parteBaja.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!parteBaja.isBypassed<2>())
                mag *= parteBaja.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!parteBaja.isBypassed<3>())
                mag *= parteBaja.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!cadena.isBypassed<PosicionCadenas::Alto>())
        {
            if (!parteAlta.isBypassed<0>())
                mag *= parteAlta.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!parteAlta.isBypassed<1>())
                mag *= parteAlta.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!parteAlta.isBypassed<2>())
                mag *= parteAlta.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!parteAlta.isBypassed<3>())
                mag *= parteAlta.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        mags[i] = Decibels::gainToDecibels(mag);
    }

    onda.clear();

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    onda.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        onda.lineTo(responseArea.getX() + i, map(mags[i]));
    }
}

void ComponenteAnalizador::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Rellenamos el findo con un color opaco)
    g.fillAll(Colours::black);

    drawBackgroundGrid(g);

    auto responseArea = getAnalysisArea();

    if (shouldShowFFTAnalysis)
    {
        auto señalFFTCanalIzq = productorOndaIzq.getPath();
        señalFFTCanalIzq.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

        g.setColour(Colour(73u, 243u, 242u)); //turquesa neon
        g.strokePath(señalFFTCanalIzq, PathStrokeType(1.f));

        auto señalFFTCanalDer = productorOndaDer.getPath();
        señalFFTCanalDer.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

        g.setColour(Colour(255u, 20u, 20u));//Rojo
        g.strokePath(señalFFTCanalDer, PathStrokeType(1.f));
    }

    g.setColour(Colours::white);
    g.strokePath(onda, PathStrokeType(2.f));

    Path border;

    border.setUsingNonZeroWinding(false);

    border.addRoundedRectangle(getRenderArea(), 4);
    border.addRectangle(getLocalBounds());

    g.setColour(Colours::black);

    g.fillPath(border);

    drawTextLabels(g);

    g.setColour(Colours::lavenderblush);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
}

std::vector<float> ComponenteAnalizador::getFrequencies()
{
    return std::vector<float>
    {
        20, /*30, 40,*/ 50, 100,
            200, /*300, 400,*/ 500, 1000,
            2000, /*3000, 4000,*/ 5000, 10000,
            20000
    };
}

std::vector<float> ComponenteAnalizador::getGains()
{
    return std::vector<float>
    {
        -24, -12, 0, 12, 24
    };
}

std::vector<float> ComponenteAnalizador::getXs(const std::vector<float>& freqs, float left, float width)
{
    std::vector<float> xs;
    for (auto f : freqs)
    {
        auto normX = juce::mapFromLog10(f, 20.f, 20000.f);
        xs.push_back(left + width * normX);
    }

    return xs;
}

void ComponenteAnalizador::drawBackgroundGrid(juce::Graphics& g)
{
    using namespace juce;
    auto freqs = getFrequencies();

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    auto xs = getXs(freqs, left, width);

    g.setColour(Colours::dimgrey);
    for (auto x : xs)
    {
        g.drawVerticalLine(x, top, bottom);
    }

    auto gain = getGains();

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }
}

void ComponenteAnalizador::drawTextLabels(juce::Graphics& g)
{
    using namespace juce;
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();

    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    auto freqs = getFrequencies();
    auto xs = getXs(freqs, left, width);

    for (int i = 0; i < freqs.size(); ++i)
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if (f > 999.f)
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if (addK)
            str << "k";
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;

        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    auto gain = getGains();

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        String str;
        if (gDb > 0)
            str << "+";
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);

        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);

        str.clear();
        str << (gDb - 24.f);

        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
    }
}

void ComponenteAnalizador::resized()
{
    using namespace juce;

    onda.preallocateSpace(getWidth() * 3);
    actualizaSeñal();
}

void ComponenteAnalizador::parameterValueChanged(int parameterIndex, float newValue)
{
    parametrosModificados.set(true);
}

void ProductorDeOndas::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    while (canalIzqFIFO->getNumCompleteBuffersAvailable() > 0)
    {
        if (canalIzqFIFO->getAudioBuffer(tempIncomingBuffer))
        {
            auto size = tempIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                monoBuffer.getReadPointer(0, size),
                monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                tempIncomingBuffer.getReadPointer(0, 0),
                size);

            generadorDatosFFTCanalIzq.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }

    const auto fftSize = generadorDatosFFTCanalIzq.getFFTSize();
    const auto binWidth = sampleRate / double(fftSize);

    while (generadorDatosFFTCanalIzq.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> datoFFT;
        if (generadorDatosFFTCanalIzq.getFFTData(datoFFT))
        {
            productorDeSeñal.generatePath(datoFFT, fftBounds, fftSize, binWidth, -48.f);
        }
    }

    while (productorDeSeñal.getNumPathsAvailable() > 0)
    {
        productorDeSeñal.getPath(señalFFTCanalIzq);
    }
}

void ComponenteAnalizador::timerCallback()
{
    if (shouldShowFFTAnalysis)
    {
        auto fftBounds = getAnalysisArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();

        productorOndaIzq.process(fftBounds, sampleRate);
        productorOndaDer.process(fftBounds, sampleRate);
    }

    if (parametrosModificados.compareAndSetBool(false, true))
    {
        updateChain();
        actualizaSeñal();
    }

    repaint();
}

void ComponenteAnalizador::updateChain()
{
    auto configuracionesCadena = getChainSettings(audioProcessor.apvts);

    cadena.setBypassed<PosicionCadenas::Bajo>(configuracionesCadena.BajoConBypass);
    cadena.setBypassed<PosicionCadenas::Pico>(configuracionesCadena.picoConBypass);
    cadena.setBypassed<PosicionCadenas::Alto>(configuracionesCadena.altoConBypass);

    auto coeficientesPico = generadorFiltroPico(configuracionesCadena, audioProcessor.getSampleRate());
    updateCoefficients(cadena.get<PosicionCadenas::Pico>().coefficients, coeficientesPico);

    auto coeficientesParteBaja = makeLowCutFilter(configuracionesCadena, audioProcessor.getSampleRate());
    auto coeficientesParteAlta = makeHighCutFilter(configuracionesCadena, audioProcessor.getSampleRate());

    updateCutFilter(cadena.get<PosicionCadenas::Bajo>(),
        coeficientesParteBaja,
        configuracionesCadena.parteBaja);

    updateCutFilter(cadena.get<PosicionCadenas::Alto>(),
        coeficientesParteAlta,
        configuracionesCadena.parteAlta);
}

juce::Rectangle<int> ComponenteAnalizador::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}


juce::Rectangle<int> ComponenteAnalizador::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}
//==============================================================================
MonitorDeEspectroDeSeñalAudioProcessorEditor::MonitorDeEspectroDeSeñalAudioProcessorEditor(MonitorDeEspectroDeSeñalAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    sliderFrecuenciaPico(*audioProcessor.apvts.getParameter("Frecuencia Pico"), "Hz"),
    sliderVolumenPico(*audioProcessor.apvts.getParameter("Volumen Pico"), "dB"),
    sliderCalidadPico(*audioProcessor.apvts.getParameter("Calidad Pico"), ""),
    sliderFrecuenciaBaja(*audioProcessor.apvts.getParameter("Frecuencia Bajo"), "Hz"),
    sliderFrecuenciaAlta(*audioProcessor.apvts.getParameter("Frecuencia Alto"), "Hz"),
    sliderPendienteBaja(*audioProcessor.apvts.getParameter("Pendiente Bajo"), "dB/Oct"),
    sliderPendienteAlta(*audioProcessor.apvts.getParameter("Pendiente Alto"), "db/Oct"),

    componenteAnalizador(audioProcessor),

    AttachmentSliderFrecuenciaPico(audioProcessor.apvts, "Frecuencia Pico", sliderFrecuenciaPico),
    AttachmentSliderVolumenPico(audioProcessor.apvts, "Volumen Pico", sliderVolumenPico),
    AttachmentSliderCalidadPico(audioProcessor.apvts, "Calidad Pico", sliderCalidadPico),
    AttachmentSliderFrecuenciaParteBaja(audioProcessor.apvts, "Frecuencia Bajo", sliderFrecuenciaBaja),
    AttachmentSliderFrecuenciaParteAlta(audioProcessor.apvts, "Frecuencia Alto", sliderFrecuenciaAlta),
    AttachmentSliderParteBaja(audioProcessor.apvts, "Pendiente Bajo", sliderPendienteBaja),
    AttachmentSliderParteAlta(audioProcessor.apvts, "Pendiente Alto", sliderPendienteAlta),

    AttachmentBypassBotonBajo(audioProcessor.apvts, "Bypass Bajo", botonBypassBajo),
    AttachmentBotonBypassPico(audioProcessor.apvts, "Bypass Pico", botonBypassPico),
    AttachmentBotonBypassAlto(audioProcessor.apvts, "Bypass Alto", botonBypassAlto),
    AttachmentBotonAnalizadorHabilitado(audioProcessor.apvts, "Analizador Activado", botonAnalizadorHabilitado)
{
    sliderFrecuenciaPico.etiquetas.add({ 0.f, "20Hz" });
    sliderFrecuenciaPico.etiquetas.add({ 1.f, "20kHz" });

    sliderVolumenPico.etiquetas.add({ 0.f, "-24dB" });
    sliderVolumenPico.etiquetas.add({ 1.f, "+24dB" });

    sliderCalidadPico.etiquetas.add({ 0.f, "0.1" });
    sliderCalidadPico.etiquetas.add({ 1.f, "10.0" });

    sliderFrecuenciaBaja.etiquetas.add({ 0.f, "20Hz" });
    sliderFrecuenciaBaja.etiquetas.add({ 1.f, "20kHz" });

    sliderFrecuenciaAlta.etiquetas.add({ 0.f, "20Hz" });
    sliderFrecuenciaAlta.etiquetas.add({ 1.f, "20kHz" });

    sliderPendienteBaja.etiquetas.add({ 0.0f, "12" });
    sliderPendienteBaja.etiquetas.add({ 1.f, "48" });

    sliderPendienteAlta.etiquetas.add({ 0.0f, "12" });
    sliderPendienteAlta.etiquetas.add({ 1.f, "48" });

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    botonBypassPico.setLookAndFeel(&lnf);
    botonBypassAlto.setLookAndFeel(&lnf);
    botonBypassBajo.setLookAndFeel(&lnf);

    botonAnalizadorHabilitado.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<MonitorDeEspectroDeSeñalAudioProcessorEditor>(this);
    botonBypassPico.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->botonBypassPico.getToggleState();

            comp->sliderFrecuenciaPico.setEnabled(!bypassed);
            comp->sliderVolumenPico.setEnabled(!bypassed);
            comp->sliderCalidadPico.setEnabled(!bypassed);
        }
    };


    botonBypassBajo.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->botonBypassBajo.getToggleState();

            comp->sliderFrecuenciaBaja.setEnabled(!bypassed);
            comp->sliderPendienteBaja.setEnabled(!bypassed);
        }
    };

    botonBypassAlto.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->botonBypassAlto.getToggleState();

            comp->sliderFrecuenciaAlta.setEnabled(!bypassed);
            comp->sliderPendienteAlta.setEnabled(!bypassed);
        }
    };

    botonAnalizadorHabilitado.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto enabled = comp->botonAnalizadorHabilitado.getToggleState();
            comp->componenteAnalizador.toggleAnalysisEnablement(enabled);
        }
    };

    setSize(480, 500);
}

MonitorDeEspectroDeSeñalAudioProcessorEditor::~MonitorDeEspectroDeSeñalAudioProcessorEditor()
{
    botonBypassPico.setLookAndFeel(nullptr);
    botonBypassAlto.setLookAndFeel(nullptr);
    botonBypassBajo.setLookAndFeel(nullptr);

    botonAnalizadorHabilitado.setLookAndFeel(nullptr);
}

//==============================================================================
void MonitorDeEspectroDeSeñalAudioProcessorEditor::paint(juce::Graphics& g)
{
    using namespace juce;

    g.fillAll(Colours::black);

    Path curva;

    auto bounds = getLocalBounds();
    auto center = bounds.getCentre();

    g.setFont(Font("Times New Roman", 30, 0));

    String title{ "TFG - LMP" };
    g.setFont(30);
    auto titleWidth = g.getCurrentFont().getStringWidth(title);

    curva.startNewSubPath(center.x, 32);
    curva.lineTo(center.x - titleWidth * 0.45f, 32);

    auto cornerSize = 20;
    auto curvePos = curva.getCurrentPosition();
    curva.quadraticTo(curvePos.getX() - cornerSize, curvePos.getY(),
        curvePos.getX() - cornerSize, curvePos.getY() - 16);
    curvePos = curva.getCurrentPosition();
    curva.quadraticTo(curvePos.getX(), 2,
        curvePos.getX() - cornerSize, 2);

    curva.lineTo({ 0.f, 2.f });
    curva.lineTo(0.f, 0.f);
    curva.lineTo(center.x, 0.f);
    curva.closeSubPath();

    g.setColour(Colour(0, 198, 199));
    g.fillPath(curva);

    curva.applyTransform(AffineTransform().scaled(-1, 1));
    curva.applyTransform(AffineTransform().translated(getWidth(), 0));
    g.fillPath(curva);


    g.setColour(Colour(0u, 0u, 0u));//Negro
    g.drawFittedText(title, bounds, juce::Justification::centredTop, 1);

    g.setColour(Colours::grey);
    g.setFont(14);
    g.drawFittedText("Bajo", sliderPendienteBaja.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("Pico", sliderCalidadPico.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("Alto", sliderPendienteAlta.getBounds(), juce::Justification::centredBottom, 1);

    auto buildDate = Time::getCompilationDate().toString(true, false);
    auto buildTime = Time::getCompilationDate().toString(false, true);
    g.setFont(12);
    g.drawFittedText("Build: " + buildDate + "\n" + buildTime, sliderPendienteAlta.getBounds().withY(6), Justification::topRight, 2);
}

void MonitorDeEspectroDeSeñalAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(4);

    auto areaHabilitadaDelAnalizador = bounds.removeFromTop(25);

    areaHabilitadaDelAnalizador.setWidth(50);
    areaHabilitadaDelAnalizador.setX(5);
    areaHabilitadaDelAnalizador.removeFromTop(2);

    botonAnalizadorHabilitado.setBounds(areaHabilitadaDelAnalizador);

    bounds.removeFromTop(5);

    float hRatio = 25.f / 100.f; //JUCE_LIVE_CONSTANT(25) / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio); //change from 0.33 to 0.25 because I needed pico hz text to not overlap the slider thumb

    componenteAnalizador.setBounds(responseArea);

    bounds.removeFromTop(5);

    auto areaParteBaja = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto areaParteAlta = bounds.removeFromRight(bounds.getWidth() * 0.5);

    botonBypassBajo.setBounds(areaParteBaja.removeFromTop(25));
    sliderFrecuenciaBaja.setBounds(areaParteBaja.removeFromTop(areaParteBaja.getHeight() * 0.5));
    sliderPendienteBaja.setBounds(areaParteBaja);

    botonBypassAlto.setBounds(areaParteAlta.removeFromTop(25));
    sliderFrecuenciaAlta.setBounds(areaParteAlta.removeFromTop(areaParteAlta.getHeight() * 0.5));
    sliderPendienteAlta.setBounds(areaParteAlta);

    botonBypassPico.setBounds(bounds.removeFromTop(25));
    sliderFrecuenciaPico.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    sliderVolumenPico.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    sliderCalidadPico.setBounds(bounds);
}

std::vector<juce::Component*> MonitorDeEspectroDeSeñalAudioProcessorEditor::getComps()
{
    return
    {
        &sliderFrecuenciaPico,
        &sliderVolumenPico,
        &sliderCalidadPico,
        &sliderFrecuenciaBaja,
        &sliderFrecuenciaAlta,
        &sliderPendienteBaja,
        &sliderPendienteAlta,
        &componenteAnalizador,

        &botonBypassBajo,
        &botonBypassPico,
        &botonBypassAlto,
        &botonAnalizadorHabilitado
    };
}