import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader';
import { environmentMap, addToSceneAndObjects } from './animated.js';

const loader = new GLTFLoader();
const speakersPromise = new Promise((resolve, reject) => {
    loader.load('assets/krk_classic_5_studio_monitor_speaker.glb', function (glb) {
        const speakers = [];
        const leftSpeaker = glb.scene;
        leftSpeaker.envMap = environmentMap;
        leftSpeaker.castShadow = false;
        leftSpeaker.receiveShadow = false;
        leftSpeaker.scale.set(42, 42, 42);     
        leftSpeaker.position.set(-140, -20, -20);
        speakers.push(leftSpeaker);

        const rightSpeaker = leftSpeaker.clone();
        rightSpeaker.envMap = environmentMap;
        rightSpeaker.position.x += 300;

        speakers.push(rightSpeaker);

        resolve(speakers);
    }, undefined, reject)
});

const carpetPromise = new Promise((resolve, reject) => {
    loader.load('assets/fine_persian_heriz_carpet.glb', function (glb) {
        const carpet = glb.scene;
        carpet.envMap = environmentMap;
        carpet.castShadow = false;
        carpet.receiveShadow = false;
        carpet.scale.set(120, 100, 100);
        carpet.position.set(10, -98, 85);

        resolve(carpet);
    }, undefined, reject);
});

const lampPromise = new Promise((resolve, reject) => {
    loader.load('assets/floor_lamp.glb', function (glb) {
        const lamp = glb.scene;
        lamp.envMap = environmentMap;
        lamp.castShadow = false;
        lamp.receiveShadow = false;
        lamp.scale.set(112, 112, 112);
        lamp.position.set(-30, 10, -10);
        lamp.rotateY(Math.PI / 4)

        resolve(lamp);
    }, undefined, reject);
});

const soundPanelPromise = new Promise((resolve, reject) => {
    loader.load('assets/sound_proof_panel.glb', function (glb) {
        const panel = glb.scene;

        panel.envMap = environmentMap;
        panel.scale.set(5, 5, 5);
        panel.position.set(55, 45, -90);
        panel.rotateX(Math.PI / 2);

        resolve(panel);
    }, undefined, reject);
});

function addModelsToScene() {
    speakersPromise.then((speakers) => {
        speakers.forEach((speaker) => {
            addToSceneAndObjects(speaker);
        });
    });
    carpetPromise.then((carpet) => {
        addToSceneAndObjects(carpet);
    });
    lampPromise.then((lamp) => {
        addToSceneAndObjects(lamp);
    });
    soundPanelPromise.then((soundPanel) => {
        addToSceneAndObjects(soundPanel);
    });
}

export {
    addModelsToScene,
}
