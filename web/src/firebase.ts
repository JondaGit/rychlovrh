import { initializeApp } from "firebase/app";
import { doc, getFirestore } from "firebase/firestore";

const firebaseConfig = {
  apiKey: "AIzaSyDBGL9G0Zhgsbh10dUFtPZ9SqTb1WM4xa8",
  authDomain: "svistivrh.firebaseapp.com",
  projectId: "svistivrh",
  storageBucket: "svistivrh.firebasestorage.app",
  messagingSenderId: "897552727825",
  appId: "1:897552727825:web:951aa471d42aaac6cc96b0",
};

export const DEVICE_UID = "PlgfbYkQRfgUmNgrZZnUy8rfu6W2";

const app = initializeApp(firebaseConfig);
export const db = getFirestore(app);
export const deviceRef = doc(db, "devices", DEVICE_UID);
