import { createApp } from 'vue';
import App from './App.vue';
import Button from 'primevue/button';
import Card from 'primevue/card';
import Chip from 'primevue/chip';
import ConfirmPopup from 'primevue/confirmpopup';
import Divider from 'primevue/divider';
import InputSwitch from 'primevue/inputswitch';
import Knob from 'primevue/knob';
import Panel from 'primevue/panel';
import PrimeVue from 'primevue/config';
import Timeline from 'primevue/timeline';
import Toast from 'primevue/toast';

import ConfirmationService from 'primevue/confirmationservice';
import ToastService from 'primevue/toastservice';

import 'primevue/resources/themes/md-light-indigo/theme.css';
import 'primevue/resources/primevue.min.css';
import 'primeicons/primeicons.css';

const app = createApp(App);
app.use(PrimeVue, { ripple: true });

app.use(ConfirmationService);
app.use(ToastService);

app.component('Button', Button);
app.component('Card', Card);
app.component('Chip', Chip);
app.component('ConfirmPopup', ConfirmPopup);
app.component('Divider', Divider);
app.component('InputSwitch', InputSwitch);
app.component('Knob', Knob);
app.component('Panel', Panel);
app.component('Timeline', Timeline);
app.component('Toast', Toast);

app.mount('#app');
