import { createApp } from 'vue';
import App from './App.vue';
import PrimeVue from 'primevue/config';
import Button from 'primevue/button';
import Card from 'primevue/card';
import Divider from 'primevue/divider';
import Knob from 'primevue/knob';
import Timeline from 'primevue/timeline';
import InputSwitch from 'primevue/inputswitch';

import 'primevue/resources/themes/md-light-indigo/theme.css';
import 'primevue/resources/primevue.min.css';
import 'primeicons/primeicons.css';

const app = createApp(App);

app.use(PrimeVue, { ripple: true });
app.component('Button', Button);
app.component('Card', Card);
app.component('Divider', Divider);
app.component('InputSwitch', InputSwitch);
app.component('Knob', Knob);
app.component('Timeline', Timeline);

app.mount('#app');
