/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
namespace webview_plugin
{
    namespace
    {
        auto streamToVector(juce::InputStream& stream)
        {
            using namespace juce;
            std::vector<std::byte> result((size_t)stream.getTotalLength());
            stream.setPosition(0);
            [[maybe_unused]] const auto bytesRead = stream.read(result.data(), result.size());
            jassert(bytesRead == (ssize_t)result.size());
            return result;
        }

        const char* getMimeForExtension(const juce::String& extension)
        {
            static const std::unordered_map<juce::String, const char*> mimeMap =
            {
                { { "htm"   },  "text/html"                },
                { { "html"  },  "text/html"                },
                { { "txt"   },  "text/plain"               },
                { { "jpg"   },  "image/jpeg"               },
                { { "jpeg"  },  "image/jpeg"               },
                { { "svg"   },  "image/svg+xml"            },
                { { "ico"   },  "image/vnd.microsoft.icon" },
                { { "json"  },  "application/json"         },
                { { "png"   },  "image/png"                },
                { { "css"   },  "text/css"                 },
                { { "map"   },  "application/json"         },
                { { "js"    },  "text/javascript"          },
                { { "woff2" },  "font/woff2"               },
                { { "glb"   },  "model/gltf-binary"        },
                { { "gltf"  },  "model/gltf+json"          }
            };

            if (const auto it = mimeMap.find(extension.toLowerCase()); it != mimeMap.end())
                return it->second;

            jassertfalse;
            return "";
        }

        constexpr auto LOCAL_VITE_SERVER = "http://localhost:3000";

    }

    ReverbulizerAudioProcessorEditor::ReverbulizerAudioProcessorEditor(ReverbulizerAudioProcessor& p)
        : AudioProcessorEditor(&p), 
        audioProcessor(p), 
        webView{ juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
        .withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)))
        .withResourceProvider([this](const auto& url) {return getResource(url); },
                              juce::URL{LOCAL_VITE_SERVER}.getOrigin())
        .withNativeIntegrationEnabled()
        .withUserScript(R"(console.log("C++ backend here: This is run before any other loading happens.");)")
        .withInitialisationData("pluginVendor", ProjectInfo::companyName)
        .withInitialisationData("pluginName", ProjectInfo::projectName)
        .withInitialisationData("pluginVersion", ProjectInfo::versionString)
        .withNativeFunction(
            juce::Identifier{"nativeFunction"},
            [this](const juce::Array<juce::var>& args,
                juce::WebBrowserComponent::NativeFunctionCompletion completion) {
                    nativeFunction(args, std::move(completion));
            }
        )
        .withEventListener("exampleJavaScriptEvent",
            [this](juce::var objectFromFrontend) {
                labelUpdatedFromJavaScript.setText("example JavaScript event occurred with value " + objectFromFrontend.getProperty("emittedCount", 0).toString(),
                    juce::dontSendNotification);
            })}
    {

        addAndMakeVisible(webView);

        //webView.goToURL("https://google.com");
        //webView.goToURL(webView.getResourceProviderRoot());
        webView.goToURL(LOCAL_VITE_SERVER);

        runJavaScriptButton.onClick = [this] {
            constexpr auto JAVASCRIPT_TO_RUN{ "console.log(\"Hello from C++!\")" };
            webView.evaluateJavascript(
                JAVASCRIPT_TO_RUN,
                [](juce::WebBrowserComponent::EvaluationResult result) {
                    if (const auto* resultPtr = result.getResult()) {
                        std::cout << "JavaScript evaluation result: " << resultPtr->toString() << std::endl;
                    }
                    else {
                        std::cout << "JavaScript evaluation failed: " << result.getError()->message << std::endl;
                    }
                }
            );
        };
        addAndMakeVisible(runJavaScriptButton);
        

        emitJavaScriptButton.onClick = [this] {
            static const juce::Identifier EVENT_ID{ "exampleEvent" };
            webView.emitEventIfBrowserIsVisible(EVENT_ID, 42.0);
        };
        addAndMakeVisible(emitJavaScriptButton);

        addAndMakeVisible(labelUpdatedFromJavaScript);

        setResizable(true, true);
        setSize(1024, 768);
        startTimer(60);
    }

    ReverbulizerAudioProcessorEditor::~ReverbulizerAudioProcessorEditor()
    {
    }

    //==============================================================================
    void ReverbulizerAudioProcessorEditor::resized()
    {
        auto bounds = getLocalBounds();
        //const int amountToRemove = getWidth() / 2;
        webView.setBounds(bounds.removeFromRight(720));
        runJavaScriptButton.setBounds(bounds.removeFromTop(50).reduced(5));
        emitJavaScriptButton.setBounds(bounds.removeFromTop(50).reduced(5));
        labelUpdatedFromJavaScript.setBounds(bounds.removeFromTop(50).reduced(5));
    }

    void ReverbulizerAudioProcessorEditor::timerCallback()
    {
        webView.emitEventIfBrowserIsVisible("outputLevel", juce::var{});
    }

    std::optional<juce::WebBrowserComponent::Resource> ReverbulizerAudioProcessorEditor::getResource(const juce::String& url)
    {
        //static const auto resourceFileRoot = juce::File{ R"(C:\Users\Joe\source\repos\Reverbulizer\Source\ui\public)"};
        static const auto resourceDirectory = getResourceDirectory();
        const auto resourceToRetrieve = url == "/" ? "index.html" : url.fromFirstOccurrenceOf("/", false, false);

        if (resourceToRetrieve == "outputLevel.json")
        {
            juce::DynamicObject::Ptr data{ new juce::DynamicObject };
            data->setProperty("left", audioProcessor.outputLevelLeft.load());
            const auto string = juce::JSON::toString(data.get());
            juce::MemoryInputStream stream{ string.getCharPointer(),
                string.getNumBytesAsUTF8(), false };
            return juce::WebBrowserComponent::Resource{ streamToVector(stream), juce::String{"application/json"} };
        }

        if (resourceToRetrieve == "data.json")
        {
            juce::DynamicObject::Ptr data{ new juce::DynamicObject };
            data->setProperty("sampleProperty", 300.0);
            const auto string = juce::JSON::toString(data.get());
            juce::MemoryInputStream stream{ string.getCharPointer(),
                string.getNumBytesAsUTF8(), false };
            return juce::WebBrowserComponent::Resource{ streamToVector(stream), juce::String{"application/json"} };
        }

        const auto resource = resourceDirectory.getChildFile(resourceToRetrieve).createInputStream();

        if (resource)
        {
            const auto extension = resourceToRetrieve.fromLastOccurrenceOf(".", false, false);
            return juce::WebBrowserComponent::Resource{ streamToVector(*resource), getMimeForExtension(extension) };
        }

        return std::nullopt;
    }

    void ReverbulizerAudioProcessorEditor::nativeFunction(const juce::Array<juce::var>& args,
        juce::WebBrowserComponent::NativeFunctionCompletion completion)
    {
        juce::String concatenatedArgs;
        
        for (const auto& arg : args)
        {
            concatenatedArgs += arg.toString();
        }

        labelUpdatedFromJavaScript.setText("Native function called with args: " + concatenatedArgs, juce::dontSendNotification);
        completion("nativeFunction callback:: All ok!");
    }

    juce::File getResourceDirectory()
    {
        auto current = juce::File::getCurrentWorkingDirectory();

        while (current.getFileName() != juce::String(ProjectInfo::projectName))
        {
            current = current.getParentDirectory();
            jassert(current.exists());
        }

        return current.getChildFile("Source/ui/public/");
    }
}
