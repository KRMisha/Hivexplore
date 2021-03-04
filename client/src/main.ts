import { createApp } from 'vue';
import App from './App.vue';
import Button from 'primevue/button';
import Card from 'primevue/card';
import Chip from 'primevue/chip';
import Divider from 'primevue/divider';
import InputSwitch from 'primevue/inputswitch';
import Knob from 'primevue/knob';
import PrimeVue from 'primevue/config';
import Timeline from 'primevue/timeline';

import 'primevue/resources/themes/md-light-indigo/theme.css';
import 'primevue/resources/primevue.min.css';
import 'primeicons/primeicons.css';

const app = createApp(App);

app.use(PrimeVue, { ripple: true });
app.component('Button', Button);
app.component('Card', Card);
app.component('Chip', Chip);
app.component('Divider', Divider);
app.component('InputSwitch', InputSwitch);
app.component('Knob', Knob);
app.component('Timeline', Timeline);

app.mount('#app');
