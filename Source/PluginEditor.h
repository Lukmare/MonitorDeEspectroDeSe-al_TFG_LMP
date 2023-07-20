/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct GeneradorDeDatosFFT
{
    /**
     produces the FFT data from an audio buffer.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();

        datoFFT.assign(datoFFT.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, datoFFT.begin());

        // first apply a windowing function to our data
        window->multiplyWithWindowingTable(datoFFT.data(), fftSize);       // [1]

        // then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform(datoFFT.data());  // [2]

        int numBins = (int)fftSize / 2;

        //normalize the fft values.
        for (int i = 0; i < numBins; ++i)
        {
            auto v = datoFFT[i];
            //            datoFFT[i] /= (float) numBins;
            if (!std::isinf(v) && !std::isnan(v))
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            datoFFT[i] = v;
        }

        //convert them to decibels
        for (int i = 0; i < numBins; ++i)
        {
            datoFFT[i] = juce::Decibels::gainToDecibels(datoFFT[i], negativeInfinity);
        }

        datoFFTFifo.push(datoFFT);
    }

    void changeOrder(FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, datoFFT
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>

        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        datoFFT.clear();
        datoFFT.resize(fftSize * 2, 0);

        datoFFTFifo.prepare(datoFFT.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return datoFFTFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& datoFFT) { return datoFFTFifo.pull(datoFFT); }
private:
    FFTOrder order;
    BlockType datoFFT;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> datoFFTFifo;
};

template<typename PathType>
struct GeneradorDeSeñalParaAnalizador
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
        juce::Rectangle<float> fftBounds,
        int fftSize,
        float binWidth,
        float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                negativeInfinity, 0.f,
                float(bottom + 10), top);
        };

        auto y = map(renderData[0]);

        //        jassert( !std::isnan(y) && !std::isinf(y) );
        if (std::isnan(y) || std::isinf(y))
            y = bottom;

        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

        for (int binNum = 1; binNum < numBins; binNum += pathResolution)
        {
            y = map(renderData[binNum]);

            //            jassert( !std::isnan(y) && !std::isinf(y) );

            if (!std::isnan(y) && !std::isinf(y))
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;

    void drawToggleButton(juce::Graphics& g,
        juce::ToggleButton& toggleButton,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;
};

struct SliderRotatorioConEtiquetas : juce::Slider
{
    SliderRotatorioConEtiquetas(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap),
        suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~SliderRotatorioConEtiquetas()
    {
        setLookAndFeel(nullptr);
    }

    struct PosicionEtiqueta
    {
        float pos;
        juce::String etiqueta;
    };

    juce::Array<PosicionEtiqueta> etiquetas;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
private:
    LookAndFeel lnf;

    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct ProductorDeOndas
{
    ProductorDeOndas(SingleChannelSampleFifo<MonitorDeEspectroDeSeñalAudioProcessor::BlockType>& scsf) :
        canalIzqFIFO(&scsf)
    {
        generadorDatosFFTCanalIzq.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, generadorDatosFFTCanalIzq.getFFTSize());
    }
    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() { return señalFFTCanalIzq; }
private:
    SingleChannelSampleFifo<MonitorDeEspectroDeSeñalAudioProcessor::BlockType>* canalIzqFIFO;

    juce::AudioBuffer<float> monoBuffer;

    GeneradorDeDatosFFT<std::vector<float>> generadorDatosFFTCanalIzq;

    GeneradorDeSeñalParaAnalizador<juce::Path> productorDeSeñal;

    juce::Path señalFFTCanalIzq;
};

struct ComponenteAnalizador : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
    ComponenteAnalizador(MonitorDeEspectroDeSeñalAudioProcessor&);
    ~ComponenteAnalizador();

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { }

    void timerCallback() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void toggleAnalysisEnablement(bool enabled)
    {
        shouldShowFFTAnalysis = enabled;
    }
private:
    MonitorDeEspectroDeSeñalAudioProcessor& audioProcessor;

    bool shouldShowFFTAnalysis = true;

    juce::Atomic<bool> parametrosModificados{ false };

    MonoChain cadena;

    void actualizaSeñal();

    juce::Path onda;

    void updateChain();

    void drawBackgroundGrid(juce::Graphics& g);
    void drawTextLabels(juce::Graphics& g);

    std::vector<float> getFrequencies();
    std::vector<float> getGains();
    std::vector<float> getXs(const std::vector<float>& freqs, float left, float width);

    juce::Rectangle<int> getRenderArea();

    juce::Rectangle<int> getAnalysisArea();

    ProductorDeOndas productorOndaIzq, productorOndaDer;
};
//==============================================================================
struct BotonEncendido : juce::ToggleButton { };

struct BotonAnalizador : juce::ToggleButton
{
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto insetRect = bounds.reduced(4);

        randomPath.clear();

        juce::Random r;

        randomPath.startNewSubPath(insetRect.getX(),
            insetRect.getY() + insetRect.getHeight() * r.nextFloat());

        for (auto x = insetRect.getX() + 1; x < insetRect.getRight(); x += 2)
        {
            randomPath.lineTo(x,
                insetRect.getY() + insetRect.getHeight() * r.nextFloat());
        }
    }

    juce::Path randomPath;
};
/**
*/
class MonitorDeEspectroDeSeñalAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MonitorDeEspectroDeSeñalAudioProcessorEditor(MonitorDeEspectroDeSeñalAudioProcessor&);
    ~MonitorDeEspectroDeSeñalAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MonitorDeEspectroDeSeñalAudioProcessor& audioProcessor;


    SliderRotatorioConEtiquetas sliderFrecuenciaPico,
        sliderVolumenPico,
        sliderCalidadPico,
        sliderFrecuenciaBaja,
        sliderFrecuenciaAlta,
        sliderPendienteBaja,
        sliderPendienteAlta;

    ComponenteAnalizador componenteAnalizador;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment AttachmentSliderFrecuenciaPico,
        AttachmentSliderVolumenPico,
        AttachmentSliderCalidadPico,
        AttachmentSliderFrecuenciaParteBaja,
        AttachmentSliderFrecuenciaParteAlta,
        AttachmentSliderParteBaja,
        AttachmentSliderParteAlta;

    std::vector<juce::Component*> getComps();

    BotonEncendido botonBypassBajo, botonBypassPico, botonBypassAlto;
    BotonAnalizador botonAnalizadorHabilitado;

    using ButtonAttachment = APVTS::ButtonAttachment;

    ButtonAttachment AttachmentBypassBotonBajo,
        AttachmentBotonBypassPico,
        AttachmentBotonBypassAlto,
        AttachmentBotonAnalizadorHabilitado;

    LookAndFeel lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonitorDeEspectroDeSeñalAudioProcessorEditor)
};