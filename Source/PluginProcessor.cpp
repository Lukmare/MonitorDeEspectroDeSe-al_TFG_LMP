/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MonitorDeEspectroDeSeñalAudioProcessor::MonitorDeEspectroDeSeñalAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
}

MonitorDeEspectroDeSeñalAudioProcessor::~MonitorDeEspectroDeSeñalAudioProcessor()
{
}

//==============================================================================
const juce::String MonitorDeEspectroDeSeñalAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MonitorDeEspectroDeSeñalAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool MonitorDeEspectroDeSeñalAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool MonitorDeEspectroDeSeñalAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double MonitorDeEspectroDeSeñalAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MonitorDeEspectroDeSeñalAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int MonitorDeEspectroDeSeñalAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MonitorDeEspectroDeSeñalAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MonitorDeEspectroDeSeñalAudioProcessor::getProgramName(int index)
{
    return {};
}

void MonitorDeEspectroDeSeñalAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void MonitorDeEspectroDeSeñalAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;

    spec.numChannels = 1;

    spec.sampleRate = sampleRate;

    cadenaIzq.prepare(spec);
    cadenaDer.prepare(spec);

    actualizaFiltros();

    canalIzqFIFO.prepare(samplesPerBlock);
    canalDerFIFO.prepare(samplesPerBlock);

    osc.initialise([](float x) { return std::sin(x); });

    spec.numChannels = getTotalNumOutputChannels();
    osc.prepare(spec);
    osc.setFrequency(440);
}

void MonitorDeEspectroDeSeñalAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MonitorDeEspectroDeSeñalAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (//layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void MonitorDeEspectroDeSeñalAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    actualizaFiltros();

    juce::dsp::AudioBlock<float> block(buffer);

    //    buffer.clear();
    //
    //    for( int i = 0; i < buffer.getNumSamples(); ++i )
    //    {
    //        buffer.setSample(0, i, osc.processSample(0));
    //    }
    //
    //    juce::dsp::ProcessContextReplacing<float> stereoContext(block);
    //    osc.process(stereoContext);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    cadenaIzq.process(leftContext);
    cadenaDer.process(rightContext);

    canalIzqFIFO.update(buffer);
    canalDerFIFO.update(buffer);

}

//==============================================================================
bool MonitorDeEspectroDeSeñalAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MonitorDeEspectroDeSeñalAudioProcessor::createEditor()
{
    return new MonitorDeEspectroDeSeñalAudioProcessorEditor(*this);
    //    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void MonitorDeEspectroDeSeñalAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void MonitorDeEspectroDeSeñalAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        actualizaFiltros();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings configs;

    configs.frecuenciaBajo = apvts.getRawParameterValue("Frecuencia Bajo")->load();
    configs.frecuenciaAlto = apvts.getRawParameterValue("Frecuencia Alto")->load();
    configs.frecuenciaPico = apvts.getRawParameterValue("Frecuencia Pico")->load();
    configs.volumenPico = apvts.getRawParameterValue("Volumen Pico")->load();
    configs.calidadPico = apvts.getRawParameterValue("Calidad Pico")->load();
    configs.parteBaja = static_cast<Slope>(apvts.getRawParameterValue("Pendiente Bajo")->load());
    configs.parteAlta = static_cast<Slope>(apvts.getRawParameterValue("Pendiente Alto")->load());

    configs.BajoConBypass = apvts.getRawParameterValue("Bypass Bajo")->load() > 0.5f;
    configs.picoConBypass = apvts.getRawParameterValue("Bypass Pico")->load() > 0.5f;
    configs.altoConBypass = apvts.getRawParameterValue("Bypass Alto")->load() > 0.5f;

    return configs;
}

Coefficients generadorFiltroPico(const ChainSettings& configuracionesCadena, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        configuracionesCadena.frecuenciaPico,
        configuracionesCadena.calidadPico,
        juce::Decibels::decibelsToGain(configuracionesCadena.volumenPico));
}

void MonitorDeEspectroDeSeñalAudioProcessor::actualizaFiltroPico(const ChainSettings& configuracionesCadena)
{
    auto peakCoefficients = generadorFiltroPico(configuracionesCadena, getSampleRate());

    cadenaIzq.setBypassed<PosicionCadenas::Pico>(configuracionesCadena.picoConBypass);
    cadenaDer.setBypassed<PosicionCadenas::Pico>(configuracionesCadena.picoConBypass);

    updateCoefficients(cadenaIzq.get<PosicionCadenas::Pico>().coefficients, peakCoefficients);
    updateCoefficients(cadenaDer.get<PosicionCadenas::Pico>().coefficients, peakCoefficients);
}

void updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}

void MonitorDeEspectroDeSeñalAudioProcessor::actualizaFiltroBajo(const ChainSettings& configuracionesCadena)
{
    auto cutCoefficients = makeLowCutFilter(configuracionesCadena, getSampleRate());
    auto& leftLowCut = cadenaIzq.get<PosicionCadenas::Bajo>();
    auto& rightLowCut = cadenaDer.get<PosicionCadenas::Bajo>();

    cadenaIzq.setBypassed<PosicionCadenas::Bajo>(configuracionesCadena.BajoConBypass);
    cadenaDer.setBypassed<PosicionCadenas::Bajo>(configuracionesCadena.BajoConBypass);

    updateCutFilter(rightLowCut, cutCoefficients, configuracionesCadena.parteBaja);
    updateCutFilter(leftLowCut, cutCoefficients, configuracionesCadena.parteBaja);
}

void MonitorDeEspectroDeSeñalAudioProcessor::actualizaFiltroAlto(const ChainSettings& configuracionesCadena)
{
    auto highCutCoefficients = makeHighCutFilter(configuracionesCadena, getSampleRate());

    auto& leftHighCut = cadenaIzq.get<PosicionCadenas::Alto>();
    auto& rightHighCut = cadenaDer.get<PosicionCadenas::Alto>();

    cadenaIzq.setBypassed<PosicionCadenas::Alto>(configuracionesCadena.altoConBypass);
    cadenaDer.setBypassed<PosicionCadenas::Alto>(configuracionesCadena.altoConBypass);

    updateCutFilter(leftHighCut, highCutCoefficients, configuracionesCadena.parteAlta);
    updateCutFilter(rightHighCut, highCutCoefficients, configuracionesCadena.parteAlta);
}

void MonitorDeEspectroDeSeñalAudioProcessor::actualizaFiltros()
{
    auto configuracionesCadena = getChainSettings(apvts);

    actualizaFiltroBajo(configuracionesCadena);
    actualizaFiltroPico(configuracionesCadena);
    actualizaFiltroAlto(configuracionesCadena);
}

juce::AudioProcessorValueTreeState::ParameterLayout MonitorDeEspectroDeSeñalAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("Frecuencia Bajo",
        "Frecuencia Bajo",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Frecuencia Alto",
        "Frecuencia Alto",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Frecuencia Pico",
        "Frecuencia Pico",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        750.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Volumen Pico",
        "Volumen Pico",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Calidad Pico",
        "Calidad Pico",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.f));

    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("Pendiente Bajo", "Pendiente Bajo", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Pendiente Alto", "Pendiente Alto", stringArray, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>("Bypass Bajo", "Bypass Bajo", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Bypass Pico", "Bypass Pico", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Bypass Alto", "Bypass Alto", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Analizador Activado", "Analizador Activado", true));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MonitorDeEspectroDeSeñalAudioProcessor();
}